#include "shared_state.h"

// ============================================================================
// shared_state.cpp
//
// Global shared state and critical section locking implementation.
// ============================================================================

// ── Globals (file-scope) ─────────────────────────────────────────────────────
static critical_section_t s_cmd_lock;
static critical_section_t s_config_lock;
static critical_section_t s_state_lock;

static RobotCommand s_command;
static RobotConfig  s_config;
static RobotState   s_state;

// ── Init ─────────────────────────────────────────────────────────────────────
void shared_state_init() {
    critical_section_init(&s_cmd_lock);
    critical_section_init(&s_config_lock);
    critical_section_init(&s_state_lock);

    // Default-initialised via in-class initialisers in robot_state.h.
    // Nothing extra required here unless we need to override a default.
    s_command = RobotCommand{};
    s_config  = RobotConfig{};
    s_state   = RobotState{};
}

// ── RobotCommand ─────────────────────────────────────────────────────────────
RobotCommand read_command_snapshot() {
    critical_section_enter_blocking(&s_cmd_lock);
    RobotCommand copy = s_command;
    critical_section_exit(&s_cmd_lock);
    return copy;
}

void write_command(const RobotCommand& cmd) {
    critical_section_enter_blocking(&s_cmd_lock);
    s_command = cmd;
    critical_section_exit(&s_cmd_lock);
}

// ── RobotConfig ──────────────────────────────────────────────────────────────
RobotConfig read_config_snapshot() {
    critical_section_enter_blocking(&s_config_lock);
    RobotConfig copy = s_config;
    critical_section_exit(&s_config_lock);
    return copy;
}

void write_config(const RobotConfig& cfg) {
    critical_section_enter_blocking(&s_config_lock);
    s_config = cfg;
    critical_section_exit(&s_config_lock);
}

// ── RobotState ───────────────────────────────────────────────────────────────
RobotState read_state_snapshot() {
    critical_section_enter_blocking(&s_state_lock);
    RobotState copy = s_state;
    critical_section_exit(&s_state_lock);
    return copy;
}

void write_state_snapshot(const RobotState& state) {
    critical_section_enter_blocking(&s_state_lock);
    s_state = state;
    critical_section_exit(&s_state_lock);
}
