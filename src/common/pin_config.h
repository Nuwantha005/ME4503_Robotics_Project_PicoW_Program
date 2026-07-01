#pragma once

// ============================================================================
// pin_config.h
//
// Single source of truth for all GPIO pins, I2C addresses, WiFi credentials,
// and physical robot constants.  No driver file may hardcode a pin number —
// everything must come from here.
//
// Sections marked [PHASE N] are placeholders; uncomment and fill in when that
// phase is being implemented.
// ============================================================================

// ── WiFi Access Point ────────────────────────────────────────────────────────
#define WIFI_SSID          "WMR_Robot"
#define WIFI_PASSWORD      "robotwmr123"
#define WIFI_CHANNEL       6
// AP assigns itself this IP; phone gets something in 192.168.4.x range via DHCP
#define WIFI_AP_IP         "192.168.4.1"

// ── Onboard LED (Pico W) ─────────────────────────────────────────────────────
// The Pico W LED is connected to the CYW43 WiFi chip, NOT to a regular GPIO.
// Use cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1/0) from Core 0 only.
#ifndef CYW43_WL_GPIO_LED_PIN
#define CYW43_WL_GPIO_LED_PIN 0
#endif

// ── [PHASE 2] Motor Driver (TB6612FNG) ───────────────────────────────────────
// Motor A = Front-Left (FL), Motor B = Front-Right (FR)
// Motor C = Rear-Left (RL),  Motor D = Rear-Right (RR)
// #define MOTOR_FL_PWM_PIN   2
// #define MOTOR_FL_IN1_PIN   3
// #define MOTOR_FL_IN2_PIN   4
// #define MOTOR_FR_PWM_PIN   5
// #define MOTOR_FR_IN1_PIN   6
// #define MOTOR_FR_IN2_PIN   7
// #define MOTOR_RL_PWM_PIN   8
// #define MOTOR_RL_IN1_PIN   9
// #define MOTOR_RL_IN2_PIN   10
// #define MOTOR_RR_PWM_PIN   11
// #define MOTOR_RR_IN1_PIN   12
// #define MOTOR_RR_IN2_PIN   13
// #define MOTOR_STBY_PIN     14

// ── [PHASE 2] Encoders (Hall effect, quadrature) ─────────────────────────────
// #define ENC_FL_A_PIN   15
// #define ENC_FL_B_PIN   16
// #define ENC_FR_A_PIN   17
// #define ENC_FR_B_PIN   18
// #define ENC_RL_A_PIN   19
// #define ENC_RL_B_PIN   20
// #define ENC_RR_A_PIN   21
// #define ENC_RR_B_PIN   22

// ── [PHASE 3] IMU (MPU9250, I2C) ─────────────────────────────────────────────
// #define IMU_I2C_PORT   i2c0
// #define IMU_SDA_PIN    4
// #define IMU_SCL_PIN    5
// #define IMU_I2C_ADDR   0x68

// ── [PHASE 4] ToF Sensors (VL53L0X, I2C) ────────────────────────────────────
// 4 sensors share the same bus; XSHUT pins let us reassign addresses at boot.
// Front, Right, Back, Left
// #define TOF_I2C_PORT    i2c1
// #define TOF_SDA_PIN     6
// #define TOF_SCL_PIN     7
// #define TOF_XSHUT_F_PIN 8
// #define TOF_XSHUT_R_PIN 9
// #define TOF_XSHUT_B_PIN 10
// #define TOF_XSHUT_L_PIN 11
// #define TOF_ADDR_BASE   0x30  // reassigned at boot; default is 0x29

// ── [PHASE 5] IR Reflectance Array (QTR-8RC) ─────────────────────────────────
// RC-decay timing; 8 sensors, GPIO interrupt driven
// #define IR_PIN_0   26
// #define IR_PIN_1   27
// #define IR_PIN_2   28
// #define IR_PIN_3   29
// #define IR_PIN_4   0   // verify — check for conflicts with other pins
// #define IR_PIN_5   1
// #define IR_PIN_6   2
// #define IR_PIN_7   3

// ── Physical Robot Constants ─────────────────────────────────────────────────
// Placeholder values — calibrate after assembly.
#define ROBOT_WHEEL_RADIUS_M          0.040f   // 80 mm diameter → 40 mm radius
#define ROBOT_WHEELBASE_LX_M          0.060f   // half-track (lateral)
#define ROBOT_WHEELBASE_LY_M          0.050f   // half-wheelbase (longitudinal)
#define ROBOT_ENCODER_COUNTS_PER_REV  360.0f   // hall encoder PPR × 4 (quadrature)
#define ROBOT_MOTOR_GEAR_RATIO        1.0f     // update when gearbox spec confirmed
