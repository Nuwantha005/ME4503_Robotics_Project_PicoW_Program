#include "http_server.h"
#include "websocket.h"
#include "lwip/tcp.h"
#include <string.h>
#include <stdio.h>

// ============================================================================
// http_server.cpp
//
// Minimal lwIP raw TCP server.  Routes to WS handshake or static HTML page.
// ============================================================================

// ── Static status page ───────────────────────────────────────────────────────
static const char HTTP_STATUS_PAGE[] =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Connection: close\r\n"
    "\r\n"
    "<!DOCTYPE html><html><head>"
    "<title>WMR Robot</title>"
    "<meta charset=\"utf-8\">"
    "<style>body{font-family:monospace;background:#111;color:#0f0;padding:2rem;}"
    "h1{color:#0ff;}p{margin:.4rem 0;}</style>"
    "</head><body>"
    "<h1>WMR Robot — Pico W</h1>"
    "<p>Status: <b>Online</b></p>"
    "<p>Connect the Android app to <b>ws://192.168.4.1/</b></p>"
    "</body></html>\r\n";

// ── TCP callbacks ─────────────────────────────────────────────────────────────

// Per-connection state (minimal — just track whether we've handled the first request)
struct ConnState
{
    bool first_recv_done;
};

static err_t on_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
    printf("[http] Received data: %d bytes, err=%d\n", p ? p->tot_len : 0, err);

    if (!p)
    {
        // Remote closed the connection
        ws_on_close(pcb);

        tcp_arg(pcb, nullptr);
        tcp_recv(pcb, nullptr);
        tcp_err(pcb, nullptr);

        tcp_close(pcb);
        if (arg)
        {
            delete static_cast<ConnState *>(arg);
        }
        return ERR_OK;
    }

    if (err != ERR_OK)
    {
        pbuf_free(p);
        return err;
    }

    // Flatten pbuf chain into a working buffer
    static char rx_buf[2048];
    size_t total = 0;
    for (struct pbuf *q = p; q && total < sizeof(rx_buf) - 1; q = q->next)
    {
        size_t chunk = (q->len < sizeof(rx_buf) - 1 - total)
                           ? q->len
                           : sizeof(rx_buf) - 1 - total;
        memcpy(rx_buf + total, q->payload, chunk);
        total += chunk;
    }
    rx_buf[total] = '\0';
    pbuf_free(p);
    tcp_recved(pcb, (u16_t)total);

    ConnState *state = static_cast<ConnState *>(arg);

    if (!state->first_recv_done)
    {
        state->first_recv_done = true;

        printf("Starting Handshake\n");

        // Try WebSocket upgrade first
        if (ws_try_handshake(pcb, rx_buf, total))
        {
            // PCB is now a WS connection — keep it alive
            return ERR_OK;
        }

        printf("Handshake Failed, Displaying Static Page\n");

        // Otherwise serve the static page and close
        tcp_write(pcb, HTTP_STATUS_PAGE, sizeof(HTTP_STATUS_PAGE) - 1,
                  TCP_WRITE_FLAG_COPY);
        tcp_output(pcb);

        // Detach callbacks BEFORE closing — lwIP may still invoke them
        // during FIN/ACK teardown, and state won't exist anymore.
        tcp_arg(pcb, nullptr);
        tcp_recv(pcb, nullptr);
        tcp_err(pcb, nullptr);

        err_t err = tcp_close(pcb);
        printf("[http] Connection closed, err=%d\n", err);

        delete state;
        return ERR_OK;
    }

    // Subsequent data on an upgraded WS connection
    ws_feed_data(pcb, (const uint8_t *)rx_buf, total);
    return ERR_OK;
}

static void on_err(void *arg, err_t err)
{
    printf("[http] Connection error: %d\n", err);
    if (arg)
    {
        delete static_cast<ConnState *>(arg);
    }
}

static err_t on_accept(void *arg, struct tcp_pcb *new_pcb, err_t err)
{
    if (err != ERR_OK || !new_pcb)
        return ERR_VAL;

    printf("[http] New connection from client\n");

    tcp_setprio(new_pcb, TCP_PRIO_MIN);

    ConnState *state = new ConnState{};
    state->first_recv_done = false;

    tcp_arg(new_pcb, state);
    tcp_recv(new_pcb, on_recv);
    tcp_err(new_pcb, on_err);

    printf("[http] Connection established\n");
    return ERR_OK;
}

// ── Init ─────────────────────────────────────────────────────────────────────
bool http_server_init()
{
    struct tcp_pcb *listen_pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!listen_pcb)
    {
        printf("[http] ERROR: tcp_new failed\n");
        return false;
    }

    err_t err = tcp_bind(listen_pcb, IP_ANY_TYPE, 80);
    if (err != ERR_OK)
    {
        printf("[http] ERROR: tcp_bind failed: %d\n", err);
        tcp_close(listen_pcb);
        return false;
    }

    struct tcp_pcb *server_pcb = tcp_listen_with_backlog(listen_pcb, 1);
    if (!server_pcb)
    {
        printf("[http] ERROR: tcp_listen failed\n");
        tcp_close(listen_pcb);
        return false;
    }

    tcp_accept(server_pcb, on_accept);
    printf("[http] Listening on port 80\n");
    return true;
}
