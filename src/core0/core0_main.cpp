#include "core0_main.h"
#include "wifi_setup.h"
#include "websocket.h"
#include "json_protocol.h"
#include "shared_state.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include <stdio.h>
#include <string.h>

// ============================================================================
// core0_main.cpp
//
// Core 0 loop (WiFi AP, raw TCP server, WebSocket framing, JSON protocol).
//
// Establishes a tight poll loop driving CYW43, and triggers a 10 Hz
// telemetry broadcast over the active WebSocket channel.
// ============================================================================

#define TELEMETRY_BUF_SIZE 1024
static constexpr uint32_t TELEMETRY_INTERVAL_MS = 100; // 10 Hz

void core0_run()
{
    printf("[core0] Core 0 main loop running\n");

    uint32_t last_telemetry_time = to_ms_since_boot(get_absolute_time());
    static char telemetry_buf[TELEMETRY_BUF_SIZE];

    while (true)
    {
        // 1. Process WiFi / lwIP stack events (crucial in poll mode!)
        wifi_poll();

        // 2. Broadcast telemetry at 10 Hz if WebSocket client is connected
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_telemetry_time >= TELEMETRY_INTERVAL_MS)
        {
            last_telemetry_time = now;

            if (ws_is_connected())
            {
                // Read a snapshot of Core 1's telemetry/uptime
                RobotState state = read_state_snapshot();

                // Format to JSON
                if (json_protocol_build_telemetry(state, telemetry_buf, sizeof(telemetry_buf)))
                {
                    // Send to the client
                    ws_send_text(telemetry_buf, strlen(telemetry_buf));
                }
            }
        }

        // Minimal yield to prevent locking up the hardware, but keep it tight
        // so that packet reception is fast.
        tight_loop_contents();
    }
}
