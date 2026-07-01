#include "gpio_irq_dispatch.h"
#include "hardware/gpio.h"
#include <stdio.h>

// ============================================================================
// gpio_irq_dispatch.cpp  (STUB — Phase 1)
//
// Dispatcher body.  The registration table and demux callback are stubbed out
// for now because no interrupt-driven drivers exist in Phase 1.
//
// Phase 2: add encoder_isr_handler registration.
// Phase 5: add ir_qtr8rc_isr_handler registration.
// ============================================================================

static constexpr uint MAX_GPIO_HANDLERS = 32;

struct PinEntry {
    uint        pin;
    uint32_t    event_mask;
    GpioHandler handler;
    bool        active = false;
};

static PinEntry s_handlers[MAX_GPIO_HANDLERS];
static uint     s_handler_count = 0;

// Shared IRQ callback — demuxes to the registered per-pin handler.
static void dispatch_callback(uint gpio, uint32_t events) {
    for (uint i = 0; i < s_handler_count; i++) {
        if (s_handlers[i].active && s_handlers[i].pin == gpio) {
            s_handlers[i].handler(gpio, events);
            return;
        }
    }
}

void gpio_dispatch_register(uint pin, uint32_t event_mask, GpioHandler handler) {
    if (s_handler_count >= MAX_GPIO_HANDLERS) {
        // Should never happen — RP2040 has 30 user GPIO
        printf("[gpio_dispatch] ERROR: handler table full\n");
        return;
    }
    s_handlers[s_handler_count++] = { pin, event_mask, handler, true };
}

void gpio_dispatch_init() {
    for (uint i = 0; i < s_handler_count; i++) {
        if (s_handlers[i].active) {
            gpio_set_irq_enabled_with_callback(
                s_handlers[i].pin,
                s_handlers[i].event_mask,
                true,
                &dispatch_callback   // only installs the callback once for the first call;
                                     // subsequent calls to this SDK function only update the mask
            );
        }
    }

    if (s_handler_count == 0) {
        // Phase 1: nothing registered, nothing to do.
        printf("[gpio_dispatch] No handlers registered (Phase 1 stub — OK)\n");
    }
}
