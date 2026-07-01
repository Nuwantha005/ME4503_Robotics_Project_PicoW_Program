#include "core1_main.h"
#include "shared_state.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include <stdio.h>

// ============================================================================
// core1_main.cpp
//
// Core 1 control loop skeleton (Phase 1).
//
// Phase 1 scope:
//   - 100 Hz tick loop using busy_wait_until() (10 ms period).
//   - Reads uptime and writes it to RobotState so Core 0 can include it in
//     the 10 Hz telemetry broadcast.
//   - Prints a heartbeat to serial every second so we can confirm Core 1 is
//     alive during bring-up.
//
// Later phases will add:
//   - [Phase 2] gpio_dispatch_init(), encoder_read_all(), pid_update_all()
//   - [Phase 3] mecanum_inverse_kinematics(), motors_set_pwm()
//   - [Phase 4] tof_array_read(), obstacle_filter()
//   - [Phase 5] ir_array_read_and_process(), line_follow_controller()
// ============================================================================

static constexpr uint32_t TICK_US      = 10000;   // 100 Hz
static constexpr uint32_t DT_S_FLOAT  = 0.01f;

void core1_main() {
    printf("[core1] Core 1 started\n");

    // [Phase 2] gpio_dispatch_init() goes here — after all drivers have registered.

    absolute_time_t next_tick = make_timeout_time_us(TICK_US);
    uint32_t tick = 0;

    while (true) {
        busy_wait_until(next_tick);
        next_tick = delayed_by_us(next_tick, TICK_US);

        // ── Read current state ───────────────────────────────────────────
        // RobotCommand cmd = read_command_snapshot();  // [Phase 2]
        // RobotConfig  cfg = read_config_snapshot();   // [Phase 2]

        // ── Staleness check (§2): if no cmd received for >250ms, zero motion ─
        // apply_staleness_check(cmd);                  // [Phase 2]

        // ── [Phase 2] Encoder read + wheel speed computation ─────────────
        // EncoderCounts enc     = encoder_read_all();
        // WheelSpeeds measured  = compute_wheel_speeds(enc, DT_S_FLOAT);

        // ── [Phase 3] Kinematics + PID + motor output ────────────────────
        // VelocityVector target   = mode_dispatch(cmd, line_state, obstacle_state, cfg);
        // WheelSpeeds setpoints   = mecanum_inverse_kinematics(target, cfg);
        // WheelPWM pwm            = pid_update_all(setpoints, measured, DT_S_FLOAT, cfg);
        // motors_set_pwm(pwm);

        // ── Slow-sensor sub-ticks ────────────────────────────────────────
        // if (tick % 2 == 0)  line_state     = ir_array_read_and_process();  // [Phase 5] ~50 Hz
        // if (tick % 5 == 0)  obstacle_state = tof_array_read();             // [Phase 4] ~20 Hz

        // ── Write uptime to shared state (10 Hz) ─────────────────────────
        if (tick % 10 == 0) {
            RobotState state = read_state_snapshot();
            state.uptime_ms  = to_ms_since_boot(get_absolute_time());
            write_state_snapshot(state);
        }

        // ── Serial heartbeat (1 Hz) ──────────────────────────────────────
        if (tick % 100 == 0) {
            printf("[core1] tick=%lu  uptime=%lu ms\n",
                   (unsigned long)tick,
                   (unsigned long)to_ms_since_boot(get_absolute_time()));
        }

        tick++;
    }
}
