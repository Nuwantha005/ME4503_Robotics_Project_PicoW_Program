#pragma once

#include "common/robot_state.h" // RobotConfig

// Deliberately NOT a class. There is no state to own between calls —
// every call is a pure (VelocityVector, RobotConfig) -> WheelSpeeds
// transformation, computed fresh every tick from whatever config is
// current. Wrapping this in a class would just be a container for
// functions that don't share any data; that's what a namespace/free
// functions already are, at zero cost either way.

struct VelocityVector {
    float x;   // m/s, robot-frame forward/back (or field-frame if
               // resolve_manual_command() already rotated it, see §5)
    float y;   // m/s, robot-frame left/right (strafe)
    float rot; // rad/s, yaw rate
};

struct WheelSpeeds {
    float fl, fr, rl, rr; // rad/s (target, before PID)
};

// Standard parallel mecanum inverse kinematics.
// Sign convention must be verified against actual roller orientation
// once wheels are mounted (command pure strafe, confirm direction
// matches expectation) — see architecture.md §5 note, cite Li et al.
// 2019 in the report for the parallel-configuration justification.
WheelSpeeds mecanum_inverse_kinematics(const VelocityVector& v,
                                        const RobotConfig& cfg);

// Optional but worth having for the report's "Results and Discussion"
// section and for odometry (§7 stretch goal): forward kinematics,
// measured wheel speeds -> estimated robot-frame velocity. Not used in
// the hot control path, only for odom_x/y/theta bookkeeping.
VelocityVector mecanum_forward_kinematics(const WheelSpeeds& measured,
                                           const RobotConfig& cfg);
