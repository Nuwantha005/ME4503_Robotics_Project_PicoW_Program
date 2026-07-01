#pragma once

#include <cstdint>
#include "control/mecanum_kinematics.h" // VelocityVector
#include "control/PID_algo.h"           // Pid (used for steering correction)
#include "common/robot_state.h"         // RobotConfig

// This file is split into two collaborators on purpose, matching the
// hybrid deliberative/reactive pattern worth citing in the Controller
// section of the report:
//
//   LineTracker   - REACTIVE layer. Runs every IR sub-tick (~50 Hz).
//                   Pure centroid-following: reads 8 IR channels,
//                   computes error, steers. No memory of where the
//                   robot has been. Also the layer that detects
//                   junction EVENTS (it recognizes the IR pattern,
//                   it does not decide what to do about it).
//
//   PathNavigator - DELIBERATIVE layer. Only does work when a junction
//                   event fires. Owns the DFS-with-backtracking state
//                   over the branch/loop tree and turns a junction
//                   event into a maneuver command. This is the layer
//                   that actually needs to "remember" — hence a class
//                   with real state, unlike mecanum_kinematics.h.
//
// Keeping these separate means LineTracker stays cheap and testable in
// isolation (wire it to the line_position telemetry field per
// architecture.md §8 phase 5, confirm centroid tracking works, BEFORE
// PathNavigator's branch logic is layered on top).

// ---------------------------------------------------------------------
// Reactive layer
// ---------------------------------------------------------------------

enum class JunctionType : uint8_t {
    NONE,           // still just on the line, nothing to report
    BRANCH_LEFT,    // line splits, left option available
    BRANCH_RIGHT,   // line splits, right option available
    BRANCH_BOTH,    // T-junction, both options available
    DEAD_END,       // line lost / all-dark, loop end or lost line
    LOOP_CLOSE      // recognized we're back at a junction already visited
                     // this branch-descent (exact detection logic TBD —
                     // may need odometry distance, not just IR pattern)
};

struct LineTrackResult {
    int line_position;         // 0..7000, 3500 = center, -999 = invalid
                                // (matches RobotState.line_position, §2)
    float steering_error;      // signed, centroid offset from center
    JunctionType junction;     // NONE unless a junction pattern fired
};

class LineTracker {
public:
    explicit LineTracker(const RobotConfig& cfg);

    // Call on every IR sub-tick (tick % 2 in core1_main, §5).
    // raw_ir: 8 channel readings from ir_qtr8rc.cpp, already read this tick.
    LineTrackResult update(const uint16_t raw_ir[8]);

private:
    Pid steering_pid_; // centroid error -> rot correction; separate from
                        // the 4 per-wheel PIDs, this is a steering PID
    // thresholds/calibration for junction pattern recognition go here
};

// ---------------------------------------------------------------------
// Deliberative layer
// ---------------------------------------------------------------------

// Bounded by max tree depth -- fixed-size, no dynamic allocation.
// Tune to the actual course depth once it's known.
constexpr int MAX_PATH_DEPTH = 16;

enum class NavState : uint8_t {
    TRAVERSING,    // following current branch, no decision pending
    AT_JUNCTION,   // junction just detected, deciding next move
    IN_LOOP,       // committed to traversing the loop at a branch end
    BACKTRACKING,  // returning along the path already taken
    DONE           // whole tree explored (all branches + loops visited)
};

enum class Maneuver : uint8_t {
    CONTINUE,       // no action, keep line-following
    TURN_LEFT,
    TURN_RIGHT,
    U_TURN,         // e.g. reverse out of a completed loop
    STOP            // DONE, or unrecoverable (lost line + lost path)
};

// Owns the DFS-with-backtracking state. This is the class whose whole
// job is remembering where the robot has been -- unlike LineTracker or
// mecanum_kinematics, encapsulation here is doing real work, not just
// tidiness.
class PathNavigator {
public:
    PathNavigator();

    // Call only when LineTracker reports junction != NONE. Returns the
    // maneuver to execute; the reactive layer resumes centroid-following
    // once the maneuver completes.
    Maneuver on_junction(JunctionType junction);

    NavState state() const { return state_; }

    // For telemetry / debugging (e.g. a test screen, §6) -- expose
    // enough to see the DFS stack without exposing the stack itself.
    int current_depth() const { return depth_; }

private:
    NavState state_ = NavState::TRAVERSING;

    // DFS stack: which branch choice was taken at each depth, so
    // backtracking knows what to undo.
    Maneuver path_stack_[MAX_PATH_DEPTH];
    bool visited_[MAX_PATH_DEPTH][2]; // [depth][left/right] visited flags
    int depth_ = 0;

    Maneuver decide_at_junction(JunctionType junction);
    Maneuver decide_backtrack_step();
};
