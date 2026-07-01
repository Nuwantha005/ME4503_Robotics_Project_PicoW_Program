#pragma once

#include <stdbool.h>

// ============================================================================
// http_server.h
//
// Raw lwIP TCP server on port 80.
//
// First received data on a new connection is inspected:
//   - If it contains "Upgrade: websocket" → WebSocket handshake (websocket.h)
//   - Otherwise                           → serve a minimal static HTML page
//
// After the WebSocket upgrade, all subsequent data on that TCP connection is
// fed to ws_feed_data() for frame decoding and JSON routing.
//
// Policy: one active client at a time.  A new TCP accept while a WS client is
// connected closes the old connection (ws_on_close) before accepting the new.
// ============================================================================

// Start listening on port 80.  Must be called after wifi_init().
// Returns true on success.
bool http_server_init();
