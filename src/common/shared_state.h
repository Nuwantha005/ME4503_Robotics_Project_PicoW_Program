#pragma once

#include "robot_state.h"
#include "pico/sync.h"

// ============================================================================
// shared_state.h
//
// Cross-core shared state: one critical section per struct (keeping sections
// short and avoiding unrelated writers blocking each other).
//
// LOCKING PATTERN — both directions, always:
//
//   RobotCommand read_command_snapshot() {
//       critical_section_enter_blocking(&cmd_lock);
//       RobotCommand copy = g_command;
//       critical_section_exit(&cmd_lock);
//       return copy;
//   }
//
// Never hold a lock while doing I/O, math, or anything that can stall.
// Copies are tens of bytes; the lock hold time is hundreds of nanoseconds.
//
// Call shared_state_init() once from main() before launching Core 1.
// ============================================================================

// ── Initialise ───────────────────────────────────────────────────────────────
// Initialises all three critical sections and sets default values on the
// global structs.  Must be called from Core 0 before multicore_launch_core1().
void shared_state_init();

// ── RobotCommand (Core 0 writes, Core 1 reads) ───────────────────────────────
RobotCommand read_command_snapshot();
void         write_command(const RobotCommand& cmd);

// ── RobotConfig  (Core 0 writes, Core 1 reads) ───────────────────────────────
RobotConfig  read_config_snapshot();
void         write_config(const RobotConfig& cfg);

// ── RobotState   (Core 1 writes, Core 0 reads) ───────────────────────────────
RobotState   read_state_snapshot();
void         write_state_snapshot(const RobotState& state);
