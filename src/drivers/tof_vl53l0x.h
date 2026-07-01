#pragma once

#include <cstdint>

// One file for all 4 physical VL53L0X sensors (AGENT.md: "one driver
// file per peripheral TYPE, not per instance"). All 4 share an I2C bus,
// which is itself a reason NOT to model these as 4 independent objects
// -- addressing/multiplexing is naturally a module-level concern
// (which XSHUT pin to assert to change an address at init time, etc.),
// not something that cleanly factors into 4 separate instances anyway.

constexpr int NUM_TOF = 4; // order: front=0, right=1, back=2, left=3
                            // (matches RobotState.tof_mm order, §2)

struct TofReadings {
    uint16_t mm[NUM_TOF]; // 0 or a very large sentinel value = invalid/
                           // out-of-range reading, define the sentinel
                           // and use it consistently -- obstacle_filter()
                           // needs to distinguish "no obstacle" from
                           // "sensor fault"
};

// One-time init: I2C bus setup, per-sensor address reassignment via
// XSHUT sequencing (all 4 boot at the same default I2C address --
// this is the fiddly part, budget time for it).
void tof_init();

// Called on the ToF sub-tick (tick % 5, ~20 Hz per architecture.md §5).
// Reading all 4 in one call, not 4 separate calls, keeps the I2C bus
// access pattern in one place and matches the sub-rate concept cleanly.
TofReadings tof_array_read();
