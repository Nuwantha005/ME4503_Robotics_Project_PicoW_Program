#pragma once

// ============================================================================
// core0_main.h
//
// Entry point for Core 0's main loop.
//
// Core 0 runs the CYW43 poll loop and drives HTTP / WebSocket routing.
// It also periodically reads RobotState to publish telemetry broadcasts
// over WebSocket at 10 Hz.
// ============================================================================

// Main loop for Core 0. Called from main.cpp after init. Never returns.
void core0_run();
