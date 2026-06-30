---
title: Project Description - Autonomous WMR
status: shared reference, include in both firmware and Android app repos
---

# Autonomous Wheeled Mobile Robot (WMR)

University project (ME4503, Semester 8, individual, 40% of module grade, includes
a live demo and a viva). The deliverable is a four-wheel mecanum-drive robot
controlled from an Android app over WiFi, capable of manual teleoperation,
autonomous line following, and toggleable obstacle avoidance.

## The two halves

This robot is built from two repositories that talk to each other over a
WebSocket connection on the robot's own WiFi access point:

1. **Firmware repo** — bare-metal C++ on a Raspberry Pi Pico W (RP2040, Pico
   SDK). Runs the WiFi/WebSocket server, the control loop, sensor drivers,
   and PID/kinematics. Pico W broadcasts its own AP (no router dependency,
   for demo reliability).
2. **Android app repo** — Android Studio app. Sends teleop commands and
   config updates, displays live telemetry and sensor dashboards, hosts a
   set of isolated test screens used during bring-up of individual hardware
   modules.

Both repos must agree on a single shared communication contract — message
types, field names, and units — documented in the firmware repo's
`architecture.md` under "Communication Protocol." Treat that section as the
source of truth; if the protocol needs to change, change it there first,
then update both clients to match.

## Hardware summary

4× 80mm mecanum wheels, 4× N20 6V 300RPM motors w/ hall encoders, 2×
TB6612FNG dual motor drivers, MPU9250 IMU, 4× VL53L0X ToF sensors, QTR-8RC
8-channel IR reflectance array, 2S 18650 pack w/ BMS, Raspberry Pi Pico W.

## Core capabilities being built (in rough build order)

1. WiFi AP + WebSocket server, proven against a trivial LED-toggle test screen.
2. Per-wheel PID closing the loop on encoder-measured speed, proven against
   a motor RPM test screen with live gain tuning.
3. Mecanum inverse kinematics + multi-scheme manual drive (buttons,
   joystick, phone tilt, field-centric).
4. Obstacle-detection filter (ToF-based, threshold-configurable, applies in
   any mode — not a mode itself).
5. Line-following controller (QTR-8RC, interrupt-driven RC-decay timing).
6. Telemetry dashboards (IR bar graph, speed/accel graphs, ToF proximity
   HUD, raw sensor readout).
7. Mapping / odometry display, and arbitrary waypoint path following
   (stretch goal, adds a new firmware mode).

## Design principles carried across both repos

- **Build and test one feature at a time, using the app as the test
  harness**, rather than integrating the whole stack before anything is
  verified end-to-end.
- **Mode vs. toggle are different things.** `mode` (`MANUAL` /
  `LINE_FOLLOW` / later `PATH_FOLLOW`) selects who generates the velocity
  command. Obstacle detection is a downstream toggle with a configurable
  distance threshold that can filter the command regardless of mode — it is
  not a mode itself.
- **Within `MANUAL` mode, the control scheme is just the source of the
  velocity vector** (buttons / joystick / tilt / field-centric) — the
  firmware doesn't care which one produced it, except field-centric, which
  needs the firmware's own IMU yaw to do the coordinate rotation (see
  `architecture.md`).
- **High-frequency teleop data and low-frequency tunable config are sent as
  separate message types**, not bundled together, since they have very
  different update rates and consequences if dropped.
