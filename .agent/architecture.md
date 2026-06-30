# Architecture

This is the authoritative technical design for the WMR firmware. See
`project_description.md` for project context and `AGENT.md` for repo rules.
If the Android app and firmware ever disagree on a message format, this
document — specifically "Communication Protocol" — is the source of truth;
fix this doc first, then update both repos to match.

## 1. System overview

Two cores, fixed ownership, no shared peripheral access between them:

- **Core 0** — WiFi + WebSocket server. Owns the CYW43 WiFi chip and lwIP.
  Touches no other peripheral. Runs `cyw43_arch_poll()` continuously
  (event-driven command intake) and emits a telemetry JSON message at a
  fixed ~10 Hz.
- **Core 1** — control loop. Owns every physical peripheral: motors,
  encoders, IMU, ToF array, IR array. Runs a fixed 100 Hz (10 ms) tick for
  PID/kinematics/mode-dispatch, with slower peripherals (IR, ToF) read on
  sub-divided ticks rather than every cycle.

The only crossing point between cores is a small set of shared structs
protected by `critical_section_t`, described below. Neither core calls
into the other's owned peripherals or libraries directly.

Reasoning for AP mode over joining an existing router: no dependency on lab
WiFi credentials, no DHCP delay, no router admin blocking discovery, fixed
known IP for the phone to connect to. Worth stating as a deliberate demo-
reliability design decision in the report.

## 2. Shared state structs (`common/robot_state.h`)

Four structs, kept deliberately separate because they have different
update rates, different producers, and different consequences if stale:

```cpp
enum class Mode : uint8_t { MANUAL, LINE_FOLLOW /*, PATH_FOLLOW later */ };
enum class ControlScheme : uint8_t { BUTTONS, JOYSTICK, TILT, FIELD_CENTRIC };

// High-frequency teleop input. Written by Core 0 on every inbound "cmd"
// message, read every tick by Core 1. ~10-30 Hz from the app while a
// control screen is actively being touched; otherwise unchanged.
struct RobotCommand {
    Mode mode;
    ControlScheme control_scheme;     // only meaningful when mode == MANUAL
    float x, y, rot;                  // -1..1, robot-frame unless control_scheme == FIELD_CENTRIC
    bool obstacle_enabled;
    uint32_t last_update_ms;          // for staleness check, see below
};

// Low-frequency tunables. Written by Core 0 only when the app's Config tab
// sends a change (not every loop), read every tick by Core 1. Defaults
// live in firmware; app reads current values on connect.
struct RobotConfig {
    // surface-level
    float max_speed_mps;
    float max_accel_mps2;
    float obstacle_stop_distance_mm;
    // micro-managing
    float pid_kp, pid_ki, pid_kd;     // single gain set applied to all 4 wheel PIDs
    float wheel_radius_m;
    float wheelbase_lx_m, wheelbase_ly_m;   // half-track, half-wheelbase -> L = lx + ly
    float encoder_counts_per_rev;
    float motor_gear_ratio;
};

// Telemetry out. Written by Core 1 at the end of each tick (cheap fields)
// and on the slow-sensor ticks (IR/ToF fields), read by Core 0 at 10 Hz to
// publish over WebSocket.
struct RobotState {
    float wheel_speed_fl, wheel_speed_fr, wheel_speed_rl, wheel_speed_rr; // measured, rad/s
    float odom_x_m, odom_y_m, odom_theta_rad;
    uint16_t ir_decay_us[8];
    uint16_t tof_mm[4];               // front, right, back, left
    int line_position;                // 0..7000, 3500 = center, -999 = invalid/no-line
    float battery_voltage;
    uint32_t uptime_ms;
};

// Occasional, larger payload — sent rarely (path upload), not part of the
// 100Hz hot path. Stretch goal feature, see §7.
struct Waypoint { float x_m, y_m; };
```

### Locking pattern

Each struct gets its own `critical_section_t` (don't share one lock across
all four — keeps critical sections short and avoids unrelated writers
blocking each other). Both directions follow "snapshot, copy out, release
immediately":

```cpp
RobotCommand read_command_snapshot() {
    critical_section_enter_blocking(&cmd_lock);
    RobotCommand copy = g_command;
    critical_section_exit(&cmd_lock);
    return copy;
}
```

These struct copies are tens of bytes and take on the order of hundreds of
nanoseconds — at a 10 ms Core 1 tick period that's roughly 0.01% of the
budget. Don't over-engineer this with lock-free double-buffering; it's not
needed at this scale and isn't worth the added complexity for a graded
project unless a marker specifically probes for it.

### Staleness handling

`RobotCommand.last_update_ms` is set by Core 0 whenever a new "cmd" message
is processed. Core 1 checks, every tick: if
`now_ms - cmd.last_update_ms > 250`, treat `x, y, rot` as zero regardless of
their stored values — this is the only real cross-core synchronization
logic in the system (everything else is "read whatever's latest, no
handshake needed"), and it's a safety requirement, not an optimization:
without it, a dropped WiFi connection means the robot keeps executing the
last command it received forever.

## 3. Communication protocol (WebSocket + JSON)

Single TCP listener on port 80. First line of any request is inspected: if
it carries `Upgrade: websocket`, perform the WS handshake (SHA-1 of
`Sec-WebSocket-Key` + GUID `258EAFA5-E914-47DA-95CA-C5AB0DC85B11`, base64,
return in `101 Switching Protocols`); otherwise serve the static app page.
lwIP runs in poll mode (`cyw43_arch_poll()` from the Core 0 loop) — not
`threadsafe_background` — since there's nothing else competing for Core 0
and poll mode avoids needing `cyw43_arch_lwip_begin/end` wrapping
everywhere. Framing only implements payload lengths `<126` and `==126`
(2-byte extended length) — our JSON payloads never approach 64KB, so the
8-byte extended-length case is intentionally unimplemented.

All messages are JSON objects with a `"type"` field for routing. Every
message type below is a stable contract between the firmware and the app —
changing a field name or unit must be a coordinated change on both sides.

**App → Pico:**

```jsonc
// "cmd" — high frequency, sent while a control screen is actively touched
{"type":"cmd","mode":"manual","scheme":"joystick","x":0.6,"y":-0.2,"rot":0.0,"obstacle_enabled":true}

// "config" — low frequency, sent on Config tab edits/save
{"type":"config","max_speed_mps":0.8,"max_accel_mps2":1.5,"obstacle_stop_distance_mm":150,
 "pid_kp":1.2,"pid_ki":0.05,"pid_kd":0.01,
 "wheel_radius_m":0.04,"wheelbase_lx_m":0.06,"wheelbase_ly_m":0.05,
 "encoder_counts_per_rev":360,"motor_gear_ratio":1.0}

// "waypoints" — stretch goal, infrequent, see §7
{"type":"waypoints","points":[{"x":0.5,"y":0.0},{"x":0.5,"y":0.5}]}

// "test" — generic passthrough for hamburger-menu test screens, see §6
{"type":"test","target":"led","action":"toggle"}
{"type":"test","target":"motor_fl","action":"set_pwm","value":0.3}
```

**Pico → App:**

```jsonc
// "telemetry" — ~10 Hz
{"type":"telemetry","wheel_speeds":[12.1,12.3,11.9,12.0],
 "odom":{"x":0.42,"y":-0.1,"theta":0.05},
 "ir":[120,118,125,2400,2410,130,122,119],"line_position":3550,
 "tof":[400,1200,2000,800],"battery_v":7.6,"uptime_ms":184223}

// "test_result" — response to a "test" message, shown in the test screen's debug field
{"type":"test_result","target":"led","state":"on"}
```

Use `cJSON` for all of this (`external/cjson`) — don't hand-write a parser.
Keep `RobotCommand`/`RobotConfig`/`RobotState` and these JSON shapes in
sync explicitly; a small `core0/json_protocol.cpp` should be the only place
that converts between them in either direction, so a schema change only
touches one file.

## 4. GPIO interrupt dispatch (`common/gpio_irq_dispatch.h`)

The RP2040 SDK has **one shared GPIO IRQ callback per core**, not one per
pin or per module. Calling `gpio_set_irq_enabled_with_callback()` from more
than one driver silently overwrites the previous handler — e.g. the IR
driver and the encoder driver both need GPIO interrupts, and whichever
initializes last would otherwise win, silently breaking the other.

Fix: a single shared dispatcher that every interrupt-driven module
registers into instead of calling the SDK function directly.

```cpp
using GpioHandler = void(*)(uint gpio, uint32_t events);
void gpio_dispatch_register(uint pin, uint32_t event_mask, GpioHandler handler);
void gpio_dispatch_init(); // call once, in core1_main, after all modules have registered
```

`ir_qtr8rc.cpp` and `encoder.cpp` both register their pins through this;
the dispatcher demuxes on the real GPIO IRQ vector and calls the right
module's handler. Any future interrupt-driven driver must use this too —
never call `gpio_set_irq_enabled_with_callback` a second time anywhere in
the codebase.

ISR rules: keep them to timestamp/counter writes only, no math, no
blocking calls — anything an ISR touches must be `volatile` so the compiler
doesn't cache a stale value across the interruption point.

## 5. Core 1 control loop

```cpp
void core1_main() {
    gpio_dispatch_init();
    absolute_time_t next_tick = make_timeout_time_us(10000); // 100 Hz
    uint32_t tick = 0;

    while (true) {
        busy_wait_until(next_tick);
        next_tick = delayed_by_us(next_tick, 10000);

        EncoderCounts enc = encoder_read_all();
        WheelSpeeds measured = compute_wheel_speeds(enc, DT_S);
        RobotCommand cmd = read_command_snapshot();
        RobotConfig cfg = read_config_snapshot();
        apply_staleness_check(cmd);                       // §2

        VelocityVector target = mode_dispatch(cmd, line_state, obstacle_state, cfg);
        WheelSpeeds setpoints = mecanum_inverse_kinematics(target, cfg);
        WheelPWM pwm = pid_update_all(setpoints, measured, DT_S, cfg);
        motors_set_pwm(pwm);

        if (tick % 2 == 0) line_state = ir_array_read_and_process();      // ~50 Hz
        if (tick % 5 == 0) obstacle_state = tof_array_read();             // ~20 Hz
        if (tick % 10 == 0) write_state_snapshot(measured, line_state, obstacle_state); // 10 Hz

        tick++;
    }
}
```

### Mode dispatch

```cpp
VelocityVector mode_dispatch(const RobotCommand& cmd, const LineState& line,
                              const ObstacleState& obstacle, const RobotConfig& cfg) {
    VelocityVector raw = (cmd.mode == Mode::MANUAL)
        ? resolve_manual_command(cmd)        // see control schemes below
        : line_follow_controller(line, cfg);

    return cmd.obstacle_enabled
        ? obstacle_filter(raw, obstacle, cfg.obstacle_stop_distance_mm)
        : raw;
}
```

`mode` picks the source of the velocity vector. `obstacle_enabled` is an
independent, always-applicable toggle downstream of whichever mode
produced the raw vector — this is deliberate, not an accident of code
structure: obstacle avoidance should be able to clamp/zero forward motion
in manual mode just as much as during line following.

### Control schemes (within `MANUAL`)

All schemes ultimately produce the same `{x, y, rot}` struct — the
firmware is otherwise blind to which UI generated it:

```cpp
VelocityVector resolve_manual_command(const RobotCommand& cmd) {
    VelocityVector v{cmd.x, cmd.y, cmd.rot};
    if (cmd.control_scheme == ControlScheme::FIELD_CENTRIC) {
        float yaw = imu_get_yaw_rad();           // firmware's own heading, not the app's
        float cos_y = cosf(-yaw), sin_y = sinf(-yaw);
        float fx = v.x * cos_y - v.y * sin_y;
        float fy = v.x * sin_y + v.y * cos_y;
        v.x = fx; v.y = fy;
    }
    return v;                                    // BUTTONS / JOYSTICK / TILT pass through unchanged
}
```

Field-centric rotation is done on the firmware, not the app — the
firmware owns the IMU and has the authoritative current heading; doing the
rotation app-side would require streaming yaw to the app fast enough to
stay in sync, which is strictly worse. Buttons, joystick, and tilt are all
just different *input methods* on the app side that all resolve to the
same `x, y, rot` payload before they ever leave the phone — the firmware
doesn't need a case for each of those three.

### Mecanum inverse kinematics

```cpp
WheelSpeeds mecanum_inverse_kinematics(const VelocityVector& v, const RobotConfig& cfg) {
    float L = cfg.wheelbase_lx_m + cfg.wheelbase_ly_m;
    WheelSpeeds w;
    w.fl = (v.x - v.y - L * v.rot) / cfg.wheel_radius_m;
    w.fr = (v.x + v.y + L * v.rot) / cfg.wheel_radius_m;
    w.rl = (v.x + v.y - L * v.rot) / cfg.wheel_radius_m;
    w.rr = (v.x - v.y + L * v.rot) / cfg.wheel_radius_m;
    return w;
}
```

Verify the sign convention against actual roller orientation once wheels
are mounted (command pure strafe, confirm direction matches expectation) —
parallel configuration per Li et al. 2019.

### PID (one instance per wheel)

Derivative-on-measurement (not derivative-on-error) to avoid output spikes
on setpoint jumps (e.g. a flicked joystick); anti-windup clamps the
integral term to the actuator's output range divided by `ki`. All four
wheels currently share a single gain set from `RobotConfig` — if per-wheel
tuning turns out to be needed later, that's a straightforward struct
change (array of 4 gain sets instead of one), not an architectural one.

## 6. Test harness passthrough

The Android app's hamburger menu hosts isolated single-feature test
screens (WiFi/LED toggle, motor PWM + live PID slider, etc.) used during
bring-up, reusing the same WebSocket connection as normal operation. These
are served by the `"test"` / `"test_result"` message types (§3) routed in
`core0/json_protocol.cpp` to small handler functions in the relevant
driver module (e.g. `motor_tb6612.cpp` exposes `motor_test_set_pwm()`).
Test messages bypass `mode_dispatch` entirely — they talk directly to a
driver, which is intentional: these screens exist specifically to validate
one peripheral in isolation, without the rest of the control loop in the
way. As each new sensor/actuator type is integrated, give it a
corresponding `"test"` target before wiring it into the main loop.

## 7. Stretch goal: arbitrary path following

Not part of the MVP. If pursued: add `Mode::PATH_FOLLOW`, a `Waypoint`
list populated from the `"waypoints"` message (§3), and a path-follow
controller analogous to `line_follow_controller` that consumes
`odom_x_m/odom_y_m/odom_theta_rad` from `RobotState` and the waypoint list
to produce a `VelocityVector`. Odometry itself (dead-reckoning from wheel
speeds + heading) needs to exist and be reasonably accurate before this is
worth attempting — treat it as a separate, later module, not bundled into
the first path-following pass.

## 8. Build phases (suggested order)

1. Project skeleton + WiFi/WebSocket/JSON pipeline, proven via LED test
   screen only (see `starting_prompt.md`).
2. Encoders + per-wheel PID, proven via motor PWM/RPM test screen with
   live gain editing from the app's Config tab.
3. Mecanum kinematics + manual drive, all four control schemes, proven via
   the app's Control tab.
4. Obstacle filter (ToF), threshold adjustable from Config, testable in
   manual mode before line following exists.
5. Line following (QTR-8RC, interrupt-driven), proven via the line
   position telemetry field before trusting it to drive the robot
   autonomously.
6. Full telemetry dashboards (Meters/Telemetry tabs) — IR bar graph, speed
   graphs, ToF proximity HUD, raw readout.
7. Mapping/odometry display, then path following (§7) if time allows.

Each phase should be demonstrably working over WiFi against its
corresponding app screen before starting the next — don't move to phase
N+1 with phase N only "probably working."