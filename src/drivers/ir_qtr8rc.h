#pragma once

#include <cstdint>

// QTR-8RC is already a single peripheral (8 channels on one module) --
// no "one instance per channel" question here at all, unlike ToF/
// encoders. This file's main complexity isn't the API shape, it's that
// it's interrupt-driven and MUST register through
// common/gpio_irq_dispatch.h rather than calling
// gpio_set_irq_enabled_with_callback() directly -- encoder.cpp shares
// the same GPIO IRQ vector and a second direct registration would
// silently overwrite the first (architecture.md §4).

constexpr int NUM_IR_CHANNELS = 8;

// One-time init: configure the 8 GPIO pins, register the shared IRQ
// handler through gpio_dispatch_register() (NOT
// gpio_set_irq_enabled_with_callback directly -- see architecture.md §4),
// call from core1_main before gpio_dispatch_init().
void ir_init();

// Called on the IR sub-tick (tick % 2, ~50 Hz per architecture.md §5).
// Returns raw decay times in microseconds per channel -- matches
// RobotState.ir_decay_us[8] (§2). Charge/discharge timing is handled
// by the ISR (timestamp writes only, no math -- AGENT.md ISR rules);
// this function just reads out the latest completed measurement per
// channel.
void ir_array_read(uint16_t out_decay_us[NUM_IR_CHANNELS]);

// This is the ISR itself -- kept here rather than in gpio_irq_dispatch.h
// since it's specific to this driver's charge/discharge measurement
// scheme. Registered via gpio_dispatch_register(), never called
// directly. Timestamp/counter writes only; anything it touches must be
// volatile (AGENT.md ISR rules).
void ir_gpio_irq_handler(uint gpio, uint32_t events);
