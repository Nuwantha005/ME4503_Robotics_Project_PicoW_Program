#pragma once

// Single physical sensor -- no "array of instances" question here like
// the ToF/encoder/IR drivers have. Still a free-function module, not a
// class, for the same reason as the others: core1_main just wants
// imu_get_yaw_rad() once per tick (field-centric rotation, §5), there's
// no polymorphic call site anywhere that would want an abstract
// interface. If MPU9250 access is ever swapped out (say, for a
// different IMU part), the fix is "write a new imu_XXXX.cpp with the
// same function signatures," not "add a vtable."

void imu_init(); // one-time I2C init + basic calibration, call from
                  // core1_main before the tick loop

// Call once per tick (or at whatever sub-rate you settle on -- IMU is
// cheap enough it may not need sub-division like ToF/IR do, but leave
// that decision open until you've measured the I2C transaction cost).
void imu_update();

// Authoritative heading used for field-centric rotation (architecture.md
// §5, resolve_manual_command). Radians, firmware's own integrated/fused
// estimate -- not raw magnetometer, not something the app supplies.
float imu_get_yaw_rad();

// Exposed for completeness / telemetry or future odometry work (§7) --
// not currently consumed by mode_dispatch, but cheap to have once
// you're already talking to the sensor.
float imu_get_pitch_rad();
float imu_get_roll_rad();

// Raw accel/gyro, mainly useful for the report's sensor-characterization
// section and for debugging drift -- document sensor fusion approach
// (complementary filter vs. Madgwick/Mahony, whichever you land on) in
// the "Sensors / Actuators" report section.
struct ImuRaw {
    float accel_x, accel_y, accel_z; // g
    float gyro_x, gyro_y, gyro_z;    // rad/s
    float mag_x, mag_y, mag_z;       // uT
};
ImuRaw imu_get_raw();
