#pragma once

#include "lwip/tcp.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// ============================================================================
// websocket.h
//
// Minimal RFC 6455 WebSocket implementation for the Pico W.
//
// Scope (per architecture.md §3):
//   - Handshake: SHA-1 of Sec-WebSocket-Key + GUID, base64, 101 response.
//   - Frame decode: opcode 0x1 (text), payload ≤125 bytes and 2-byte extended
//     length (126).  8-byte extended (≥126 in the length field) is deliberately
//     NOT implemented — our JSON payloads never approach that size.
//   - Frame encode: server→client text frames, unmasked (per RFC 6455 §6.1).
//   - Single client: one active WS connection at a time.
//
// The http_server drives ws_try_handshake() and ws_feed_data().
// json_protocol drives ws_send_text() to push telemetry / test_result frames.
// ============================================================================

// Attempt a WebSocket upgrade handshake on an incoming HTTP request buffer.
// Returns true and puts the PCB into WS mode if the request has
// "Upgrade: websocket".  Returns false if it's a plain HTTP request.
bool ws_try_handshake(struct tcp_pcb* pcb, const char* request, size_t len);

// Feed raw received bytes into the WS frame decoder.
// Calls json_protocol_process() internally with the decoded text payload.
void ws_feed_data(struct tcp_pcb* pcb, const uint8_t* data, size_t len);

// Send a UTF-8 text frame to the currently connected WS client.
// len must be ≤ 65535 (we support up to 2-byte extended length).
// Returns false if no client is connected or tcp_write fails.
bool ws_send_text(const char* text, size_t len);

// True if a WebSocket client is currently connected.
bool ws_is_connected();

// Call when a TCP connection closes, to clean up WS state.
void ws_on_close(struct tcp_pcb* pcb);
