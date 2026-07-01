#pragma once

#include <stdint.h>

// ============================================================================
// gpio_irq_dispatch.h
//
// Shared GPIO interrupt dispatcher for Core 1.
//
// The RP2040 SDK provides ONE shared GPIO IRQ callback per core — calling
// gpio_set_irq_enabled_with_callback() from more than one driver silently
// overwrites the previous handler.  This dispatcher multiplexes the single
// hardware callback to per-pin handlers registered by individual drivers.
//
// Usage pattern (in any interrupt-driven driver on Core 1):
//
//   // 1. Register your handler BEFORE calling gpio_dispatch_init():
//   gpio_dispatch_register(ENC_FL_A_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
//                          encoder_isr_handler);
//
//   // 2. Call once from core1_main() AFTER all modules have registered:
//   gpio_dispatch_init();
//
// NEVER call gpio_set_irq_enabled_with_callback() directly anywhere in the
// codebase — always use this interface.
//
// [PHASE 2] Encoders and [PHASE 5] IR array will register here.
// ============================================================================

#include "pico/stdlib.h"

using GpioHandler = void(*)(uint gpio, uint32_t events);

// Register a pin handler.  Must be called before gpio_dispatch_init().
// Up to 32 pins can be registered (RP2040 has 30 user GPIO, so this is safe).
void gpio_dispatch_register(uint pin, uint32_t event_mask, GpioHandler handler);

// Install the single shared IRQ callback and enable all registered pins.
// Call once from core1_main() after all drivers have called gpio_dispatch_register().
void gpio_dispatch_init();
