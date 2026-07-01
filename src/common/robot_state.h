#pragma once

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// robot_state.h
//
// Canonical shared data types for the WMR firmware.
// These structs cross the Core 0 / Core 1 boundary via critical sections in
// shared_state.h — see that file for the locking pattern.
//
// JSON field names in architecture.md §3 must stay in sync with these structs.
// The only place that converts between JSON and structs is core0/json_protocol.cpp.
// ============================================================================

// ── Operating modes ───────────────────────────────────────────────────────────
enum class Mode : uint8_t {
    MANUAL,
    LINE_FOLLOW,
    // PATH_FOLLOW  ← stretch goal, §7
};

// ── Manual control input sources ─────────────────────────────────────────────
enum class ControlScheme : uint8_t {
    BUTTONS,
    JOYSTICK,
    TILT,
    FIELD_CENTRIC,
};

// ── High-frequency teleop command ────────────────────────────────────────────
// Written by Core 0 on every inbound "cmd" message.
// Read every tick by Core 1.
// Update rate: ~10–30 Hz while a control screen is actively touched.
struct RobotCommand {
    Mode          mode            = Mode::MANUAL;
    ControlScheme control_scheme  = ControlScheme::JOYSTICK;
    float         x              = 0.0f;   // -1..1
    float         y              = 0.0f;   // -1..1
    float         rot            = 0.0f;   // -1..1
    bool          obstacle_enabled = false;
    uint32_t      last_update_ms  = 0;     // staleness guard — see architecture.md §2

    // ── Test harness fields ──
    // Set by Core 0 when a "test"→"led" message arrives; cleared by json_protocol
    // after the LED is toggled.  Kept here to avoid a separate struct/lock for
    // Phase 1 — can be refactored out if the test harness grows.
    bool          led_test_pending = false;
};

// ── Low-frequency tunables ───────────────────────────────────────────────────
// Written by Core 0 only when the app's Config tab sends a change.
// Read every tick by Core 1.
// Defaults are set in shared_state.cpp via shared_state_init().
struct RobotConfig {
    float max_speed_mps             = 0.8f;
    float max_accel_mps2            = 1.5f;
    float obstacle_stop_distance_mm = 150.0f;

    float pid_kp                    = 1.2f;
    float pid_ki                    = 0.05f;
    float pid_kd                    = 0.01f;

    float wheel_radius_m            = 0.040f;
    float wheelbase_lx_m            = 0.060f;
    float wheelbase_ly_m            = 0.050f;
    float encoder_counts_per_rev    = 360.0f;
    float motor_gear_ratio          = 1.0f;
};

// ── Telemetry / state produced by Core 1 ─────────────────────────────────────
// Written by Core 1 at the end of each tick (cheap fields) and on slow-sensor
// ticks (IR/ToF).  Read by Core 0 at 10 Hz to publish over WebSocket.
struct RobotState {
    // Wheel odometry (rad/s, measured)
    float wheel_speed_fl    = 0.0f;
    float wheel_speed_fr    = 0.0f;
    float wheel_speed_rl    = 0.0f;
    float wheel_speed_rr    = 0.0f;

    // Dead-reckoning odometry
    float odom_x_m          = 0.0f;
    float odom_y_m          = 0.0f;
    float odom_theta_rad    = 0.0f;

    // IR reflectance array (RC-decay time µs, 8 channels)
    uint16_t ir_decay_us[8] = {};

    // ToF proximity (mm): front, right, back, left
    uint16_t tof_mm[4]      = {};

    // Line follower output: 0..7000, 3500 = centred, -999 = no line
    int line_position        = -999;

    float battery_voltage    = 0.0f;
    uint32_t uptime_ms       = 0;

    // ── Test harness fields ──
    bool led_on              = false;   // actual LED state, updated by json_protocol/Core 0
};

// ── Stretch goal: waypoint path following (§7) ────────────────────────────────
struct Waypoint {
    float x_m;
    float y_m;
};
