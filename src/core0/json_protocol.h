#pragma once

#include "robot_state.h"
#include <stddef.h>

// ============================================================================
// json_protocol.h
//
// Converts between JSON strings and RobotCommand/RobotConfig/RobotState
// structures using the cJSON library.
//
// Every message type defined in architecture.md §3 is handled here.
// ============================================================================

// Process an incoming JSON string from the WebSocket client.
// Routes:
//   - {"type":"cmd",...}      → updates shared RobotCommand
//   - {"type":"config",...}   → updates shared RobotConfig
//   - {"type":"test",...}     → executes test commands (e.g. led toggle)
void json_protocol_process(const char* json_str);

// Build a JSON telemetry broadcast string based on a RobotState snapshot.
// Output format:
//   {"type":"telemetry","uptime_ms":...}  (Phase 1 minimalist format)
// Writes up to max_len bytes into out_buf. Returns true on success.
bool json_protocol_build_telemetry(const RobotState& state, char* out_buf, size_t max_len);

// Build a JSON test result response.
// Output format:
//   {"type":"test_result","target":"...","state":"..."}
// Writes up to max_len bytes into out_buf. Returns true on success.
bool json_protocol_build_test_result(const char* target, const char* result_state, char* out_buf, size_t max_len);
