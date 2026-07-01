#pragma once

#include <stdbool.h>

// ============================================================================
// led_test.h  (src/tests/)
//
// Isolated test: toggle the Pico W onboard LED via the CYW43 WiFi chip.
//
// This must only be called from Core 0, as the CYW43 SPI bus is owned by
// Core 0.  The WiFi stack (cyw43_arch) must be initialised before calling
// any function here.
//
// Call led_test_init() once from main() after wifi_init().
// Call led_test_toggle() from json_protocol.cpp when a
//   {"type":"test","target":"led","action":"toggle"} message arrives.
// ============================================================================

// Initialise LED state (off).
void led_test_init();

// Toggle the LED and return the new state (true = on, false = off).
bool led_test_toggle();

// Return the current LED state without changing it.
bool led_test_get_state();
