#pragma once

// One Pid instance per wheel (4 total, owned by core1).
// This is a genuine stateful object: integral accumulator and previous
// measurement persist across ticks. That's the whole justification for
// making this a class instead of a free function — everything else in
// control/ that DOESN'T hold state (kinematics) stays a free function.
//
// Derivative-on-measurement, not derivative-on-error: avoids output
// spikes when the setpoint jumps (e.g. a flicked joystick), since we
// differentiate the (smoother) measured signal instead of the error.
class Pid {
public:
    // kp, ki, kd: gains. All 4 wheels currently share one gain set from
    // RobotConfig (see architecture.md §2) — this ctor doesn't care,
    // it just takes whatever gains it's given.
    // output_min/max: actuator range (PWM duty, typically -1.0..1.0).
    // Used for anti-windup clamping, not just output clamping.
    Pid(float kp, float ki, float kd, float output_min, float output_max);

    // Call once per 10ms tick. Returns PWM duty for this wheel.
    // setpoint: target wheel speed, rad/s (from mecanum_inverse_kinematics)
    // measured: actual wheel speed, rad/s (from encoder_read_all)
    // dt_s: fixed at 0.01 (100 Hz), passed explicitly rather than
    //       hardcoded so this class stays testable in isolation.
    float update(float setpoint, float measured, float dt_s);

    // Called when RobotConfig changes via the app's Config tab (§3,
    // "config" message). Resetting integral on gain change avoids a
    // sudden output kick if ki changes while the integrator is loaded.
    void set_gains(float kp, float ki, float kd);

    // Call on mode transitions (e.g. MANUAL -> LINE_FOLLOW, or on
    // WiFi staleness triggering a zero command) to prevent integral
    // windup carrying over into a context it wasn't accumulated in.
    void reset();

private:
    float kp_, ki_, kd_;
    float output_min_, output_max_;

    float integral_ = 0.0f;
    float prev_measurement_ = 0.0f;
    bool has_prev_measurement_ = false; // guards the first tick's derivative
};
