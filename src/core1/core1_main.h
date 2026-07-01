#pragma once

// ============================================================================
// core1_main.h
//
// Entry point for Core 1.  Launched via multicore_launch_core1(core1_main)
// from main.cpp.  Core 1 owns all physical peripherals (motors, encoders,
// IMU, ToF, IR) — none of which exist yet in Phase 1.
// ============================================================================

// Core 1 entry point.  Never returns.
void core1_main();
