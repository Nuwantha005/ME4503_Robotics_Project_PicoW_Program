#include "websocket.h"
#include "json_protocol.h"
#include "lwip/tcp.h"
#include "mbedtls/sha1.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>

// ============================================================================
// websocket.cpp
//
// Minimal RFC 6455 WebSocket implementation.
// See websocket.h for the API contract and scope limitations.
//
// Frame format (server → client, text, unmasked):
//   Byte 0: 0x81  (FIN=1, opcode=1 text)
//   Byte 1: 0x00|len or 0x7E for extended
//   [Bytes 2-3: 16-bit big-endian length if extended]
//   [Payload bytes]
//
// Frame format (client → server, text, masked):
//   Byte 0: 0x81  (FIN=1, opcode=1 text)
//   Byte 1: 0x80|len or 0xFE for extended  (bit 7 = MASK)
//   [Bytes 2-3: 16-bit big-endian length if extended]
//   Bytes: 4-byte mask key
//   Payload XOR'd with mask key (repeating 4-byte pattern)
// ============================================================================

// ── WS Magic GUID (RFC 6455 §1.3) ───────────────────────────────────────────
static const char WS_GUID[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

// ── Base64 encoding table ────────────────────────────────────────────────────
static const char B64_TABLE[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void base64_encode(const uint8_t *in, size_t in_len, char *out, size_t out_buf_len)
{
    size_t i = 0, j = 0;
    while (i < in_len)
    {
        uint32_t octet_a = (i < in_len) ? in[i++] : 0;
        uint32_t octet_b = (i < in_len) ? in[i++] : 0;
        uint32_t octet_c = (i < in_len) ? in[i++] : 0;
        uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        if (j + 4 >= out_buf_len)
            break; // safety
        out[j++] = B64_TABLE[(triple >> 18) & 0x3F];
        out[j++] = B64_TABLE[(triple >> 12) & 0x3F];
        out[j++] = B64_TABLE[(triple >> 6) & 0x3F];
        out[j++] = B64_TABLE[triple & 0x3F];
    }
    // Padding
    size_t pad = (3 - in_len % 3) % 3;
    for (size_t p = 0; p < pad && j > 0; p++)
    {
        out[j - 1 - p] = '=';
    }
    out[j] = '\0';
}

// ── Connection state ─────────────────────────────────────────────────────────
static struct tcp_pcb *s_ws_pcb = nullptr;
static bool s_ws_connected = false;

// Reassembly buffer for fragmented TCP receives (WS frames can span packets)
static constexpr size_t RX_BUF_SIZE = 1024;
static uint8_t s_rx_buf[RX_BUF_SIZE];
static size_t s_rx_len = 0;

// ── Helpers ───────────────────────────────────────────────────────────────────

// Find a header value in an HTTP request string.
// Writes into `out` (max `out_len` bytes).  Returns true on success.
static bool find_header_value(const char *request, const char *header_name,
                              char *out, size_t out_len)
{
    const char *p = strstr(request, header_name);
    if (!p)
        return false;
    p += strlen(header_name);
    // Skip optional whitespace after the colon
    while (*p == ' ' || *p == '\t')
        p++;
    size_t i = 0;
    while (*p && *p != '\r' && *p != '\n' && i < out_len - 1)
    {
        out[i++] = *p++;
    }
    out[i] = '\0';
    return i > 0;
}

// ── Public API ────────────────────────────────────────────────────────────────

bool ws_try_handshake(struct tcp_pcb *pcb, const char *request, size_t len)
{
    // Check for WebSocket upgrade
    if (strstr(request, "Upgrade: websocket") == nullptr &&
        strstr(request, "Upgrade: WebSocket") == nullptr)
    {
        return false;
    }

    // Extract the key
    char ws_key[128] = {};
    if (!find_header_value(request, "Sec-WebSocket-Key:", ws_key, sizeof(ws_key)))
    {
        printf("[ws] ERROR: No Sec-WebSocket-Key in request\n");
        return false;
    }

    // Compute SHA-1(key + GUID)
    char combined[256];
    snprintf(combined, sizeof(combined), "%s%s", ws_key, WS_GUID);

    uint8_t sha1_out[20];
    mbedtls_sha1((const unsigned char *)combined, strlen(combined), sha1_out);

    // Base64-encode the SHA-1 digest
    char accept_key[32] = {};
    base64_encode(sha1_out, sizeof(sha1_out), accept_key, sizeof(accept_key));

    // Send 101 Switching Protocols response
    char response[512];
    int response_len = snprintf(response, sizeof(response),
                                "HTTP/1.1 101 Switching Protocols\r\n"
                                "Upgrade: websocket\r\n"
                                "Connection: Upgrade\r\n"
                                "Sec-WebSocket-Accept: %s\r\n"
                                "\r\n",
                                accept_key);

    err_t err = tcp_write(pcb, response, (u16_t)response_len, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK)
    {
        printf("[ws] ERROR: tcp_write handshake response failed: %d\n", err);
        return false;
    }
    tcp_output(pcb);

    // Mark this PCB as our active WS client, closing any previous connection
    if (s_ws_connected && s_ws_pcb && s_ws_pcb != pcb)
    {
        printf("[ws] Replacing stale WS connection\n");
        tcp_close(s_ws_pcb);
    }

    s_ws_pcb = pcb;
    s_ws_connected = true;
    s_rx_len = 0; // clear reassembly buffer

    printf("[ws] WebSocket handshake complete  (key=%s  accept=%s)\n",
           ws_key, accept_key);
    return true;
}

void ws_feed_data(struct tcp_pcb *pcb, const uint8_t *data, size_t len)
{
    if (pcb != s_ws_pcb || !s_ws_connected)
        return;

    // Append to reassembly buffer
    if (s_rx_len + len > RX_BUF_SIZE)
    {
        printf("[ws] ERROR: RX buffer overflow — dropping data\n");
        s_rx_len = 0;
        return;
    }
    memcpy(s_rx_buf + s_rx_len, data, len);
    s_rx_len += len;

    // Try to consume complete frames from the buffer
    size_t consumed = 0;
    while (s_rx_len - consumed >= 2)
    {
        const uint8_t *frame = s_rx_buf + consumed;
        size_t avail = s_rx_len - consumed;

        uint8_t opcode = frame[0] & 0x0F;
        bool fin = (frame[0] & 0x80) != 0;
        bool masked = (frame[1] & 0x80) != 0;
        uint8_t len_byte = frame[1] & 0x7F;

        size_t header_size = 2;
        size_t payload_len = 0;

        if (len_byte < 126)
        {
            payload_len = len_byte;
        }
        else if (len_byte == 126)
        {
            if (avail < 4)
                break; // wait for more data
            payload_len = ((size_t)frame[2] << 8) | frame[3];
            header_size = 4;
        }
        else
        {
            // 8-byte extended length — not supported (payloads never exceed 64KB)
            printf("[ws] ERROR: 8-byte extended payload not supported\n");
            ws_on_close(pcb);
            return;
        }

        size_t mask_size = masked ? 4 : 0;
        size_t total_frame = header_size + mask_size + payload_len;

        if (avail < total_frame)
            break; // incomplete frame, wait

        // Decode payload
        static char decoded[RX_BUF_SIZE + 1];
        if (payload_len > RX_BUF_SIZE)
        {
            printf("[ws] ERROR: payload exceeds decode buffer\n");
            consumed += total_frame;
            continue;
        }

        const uint8_t *payload = frame + header_size + mask_size;

        if (masked)
        {
            const uint8_t *mask = frame + header_size;
            for (size_t i = 0; i < payload_len; i++)
            {
                decoded[i] = (char)(payload[i] ^ mask[i % 4]);
            }
        }
        else
        {
            memcpy(decoded, payload, payload_len);
        }
        decoded[payload_len] = '\0';

        // Route by opcode
        switch (opcode)
        {
        case 0x1: // Text frame
            if (fin)
            {
                json_protocol_process(decoded);
            }
            // Fragmented text frames (fin=0) are not expected; ignore.
            break;
        case 0x8: // Close
            printf("[ws] Received close frame\n");
            ws_on_close(pcb);
            return;
        case 0x9: // Ping — respond with Pong (opcode 0xA)
        {
            uint8_t pong[2] = {0x8A, 0x00};
            tcp_write(pcb, pong, 2, TCP_WRITE_FLAG_COPY);
            tcp_output(pcb);
            break;
        }
        default:
            printf("[ws] Ignoring opcode 0x%02X\n", opcode);
            break;
        }

        consumed += total_frame;
    }

    // Shift unconsumed bytes to the front of the buffer
    if (consumed > 0 && consumed < s_rx_len)
    {
        memmove(s_rx_buf, s_rx_buf + consumed, s_rx_len - consumed);
    }
    s_rx_len -= consumed;
}

bool ws_send_text(const char *text, size_t len)
{
    if (!s_ws_connected || !s_ws_pcb)
        return false;

    // Build frame header
    uint8_t header[4];
    size_t header_len;

    header[0] = 0x81; // FIN=1, opcode=1 (text)

    if (len < 126)
    {
        header[1] = (uint8_t)len; // no mask, no extended length
        header_len = 2;
    }
    else if (len <= 65535)
    {
        header[1] = 126;
        header[2] = (uint8_t)(len >> 8);
        header[3] = (uint8_t)(len & 0xFF);
        header_len = 4;
    }
    else
    {
        printf("[ws] ERROR: payload too large to send (%zu bytes)\n", len);
        return false;
    }

    err_t err;
    err = tcp_write(s_ws_pcb, header, (u16_t)header_len, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK)
    {
        printf("[ws] tcp_write header err=%d\n", err);
        return false;
    }

    err = tcp_write(s_ws_pcb, text, (u16_t)len, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK)
    {
        printf("[ws] tcp_write payload err=%d\n", err);
        return false;
    }

    tcp_output(s_ws_pcb);
    return true;
}

bool ws_is_connected()
{
    return s_ws_connected;
}

void ws_on_close(struct tcp_pcb *pcb)
{
    if (pcb == s_ws_pcb)
    {
        s_ws_connected = false;
        s_ws_pcb = nullptr;
        s_rx_len = 0;
        printf("[ws] Connection closed\n");
    }
}
