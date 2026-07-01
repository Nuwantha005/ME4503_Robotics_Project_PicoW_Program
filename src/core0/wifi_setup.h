#pragma once

#include <stdbool.h>

// ============================================================================
// wifi_setup.h
//
// Initialises the CYW43 WiFi chip in AP mode so the robot broadcasts its
// own network.  The Pico W acts as both WiFi AP and TCP/WebSocket server.
//
// Design rationale (from architecture.md §1):
//   AP mode removes any dependency on lab WiFi credentials, DHCP delays,
//   or router admin blocks — the phone connects directly to the robot.
//   Fixed known IP (192.168.4.1) makes discovery trivial.
// ============================================================================

// Initialise CYW43 in poll mode and start the WiFi AP.
// Returns true on success, false on failure.
// Must be called from Core 0 before any other cyw43_arch_* calls.
bool wifi_init();

// Call this in the main loop to drive the CYW43 event loop.
// In poll mode there are no background interrupts — the stack only advances
// when this is called.  Never sleep or block between calls.
void wifi_poll();
