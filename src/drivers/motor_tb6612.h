#pragma once

#include <cstdint>

// Deliberately NOT a Motor class with 4 instances, and NOT an abstract
// base class shared with the sensor drivers. Nothing in the codebase
// calls motors polymorphically (core1_main calls motors_set_pwm() once,
// with all 4 PWM values -- see architecture.md §5) -- so a class
// hierarchy would exist purely for its own sake.
//
// The one place per-motor addressing is genuinely useful is the test
// harness (§6, motor PWM test screen with live PID slider). Indexed
// functions give the same ergonomics as motor(i).set_pwm() without a
// second class hierarchy to maintain alongside this one:
//
//   motor_test_set_pwm(2, 0.3f);   // vs. motors[2]->set_pwm(0.3f);
//
// Pin numbers come from common/pin_config.h, never hardcoded here --
// this file only knows "motor index 0..3", pin_config.h maps index to
// the actual TB6612FNG driver channel + GPIO pair.

constexpr int NUM_MOTORS = 4; // FL=0, FR=1, RL=2, RR=3 -- keep this
                               // ordering consistent everywhere (matches
                               // WheelSpeeds.fl/fr/rl/rr field order)

struct WheelPWM {
    float fl, fr, rl, rr; // -1.0..1.0, sign = direction
};

// One-time init: configure GPIO directions/PWM slices for all 4 motors
// from pin_config.h. Call once from core1_main before the tick loop.
void motors_init();

// Hot path: called every 10ms tick with PID output for all 4 wheels.
void motors_set_pwm(const WheelPWM& pwm);

// Immediate stop, bypassing the normal PWM path -- for the staleness
// safety check (architecture.md §2) and any fault condition.
void motors_stop_all();

// --- Test harness passthrough (§6) ---
// Talks directly to one motor, bypassing mode_dispatch/PID entirely.
// index: 0..3 (NUM_MOTORS). Out-of-range index is a no-op (log + ignore,
// don't crash core1 over a malformed test message).
void motor_test_set_pwm(uint8_t index, float pwm);
