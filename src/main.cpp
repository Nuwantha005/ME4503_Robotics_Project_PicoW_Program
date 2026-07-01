#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "shared_state.h"
#include "wifi_setup.h"
#include "http_server.h"
#include "core0_main.h"
#include "core1_main.h"
#include "led_test.h"
#include <stdio.h>

// ============================================================================
// main.cpp
//
// Root entry point for the WMR firmware.
// Wires together modules, initialises shared states/hardware, and launches
// Core 1.
// ============================================================================

int main()
{
    // 1. Initialise USB serial console (/dev/ttyACM0)
    stdio_init_all();
    // Wait briefly for serial to connect if needed (useful during debugging)
    sleep_ms(5000);
    printf("\n==================================================\n");
    printf("Starting Autonomous WMR Firmware (Phase 1)...\n");
    printf("==================================================\n");

    // 2. Initialise cross-core shared state structures & critical sections
    shared_state_init();

    // 3. Initialise WiFi Access Point mode (Core 0)
    if (!wifi_init())
    {
        printf("[main] CRITICAL ERROR: wifi_init failed!\n");
        while (true)
        {
            sleep_ms(1000);
        }
    }

    // 4. Initialise isolated LED test module
    led_test_init();

    // 5. Start HTTP/WebSocket server on Port 80
    if (!http_server_init())
    {
        printf("[main] CRITICAL ERROR: http_server_init failed!\n");
        while (true)
        {
            sleep_ms(1000);
        }
    }

    // 6. Launch Core 1 control loop
    printf("[main] Launching Core 1...\n");
    multicore_launch_core1(core1_main);

    // 7. Enters Core 0 main event loop (never returns)
    printf("[main] Entering Core 0 run loop\n");
    core0_run();

    return 0;
}
