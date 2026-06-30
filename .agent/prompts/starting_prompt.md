# Starting Prompt

We're building the firmware for a Raspberry Pi Pico W that operates a
mecanum-wheeled robot, controlled by a companion Android app over WiFi. Full
context is in `project_description.md` and `architecture.md` — read both
before writing code.

## Scope of this first phase

Don't build the whole robot yet. The goal of this phase is narrow:

1. **Project skeleton** matching the module layout in `AGENT.md` — `common/`,
   `drivers/`, `core0/`, `core1/`, each with its own `CMakeLists.txt`, wired
   together from a minimal `main.cpp` that initializes both cores and
   launches Core 1 via `multicore_launch_core1`.
2. **Core 0 WiFi + WebSocket server**, per the design in `architecture.md` →
   "Communication Protocol": Pico W runs as its own access point, serves a
   single static page on port 80, upgrades to a WebSocket connection, and
   can send/receive JSON messages (`cjson`, from `external/cjson`).
3. **Core 1 skeleton**: just enough to exist and respond — doesn't need PID,
   kinematics, or sensors yet. It should toggle the onboard LED based on a
   command received from Core 0 via the critical-section-protected
   `RobotCommand` struct (see `architecture.md` → "Shared state structs"),
   and report a heartbeat/uptime value back via `RobotState`.
4. **No real sensors or motors yet.** Those come in later phases (PID +
   encoders, then kinematics + manual drive, then line follow, then
   obstacle filter) — see `architecture.md` → "Build phases" for the full
   sequence. This phase is purely: prove the WiFi/WebSocket/JSON/dual-core
   pipeline works end-to-end, using LED-on/LED-off as the simplest possible
   payload.

## Definition of done for this phase

From the Android app's "WiFi connection test" screen (a button + a
debug-output text field — see the app's own architecture doc), tapping the
button should toggle the Pico's onboard LED, and the app should display a
telemetry message coming back from the Pico confirming the new state and an
uptime counter. That round trip — app button press → WebSocket → Core 0 →
critical section → Core 1 → LED → critical section → Core 0 → WebSocket →
app display — is what "done" means here. Everything else in
`architecture.md` builds on top of this pipeline once it's solid.

## What to discuss before writing code

Walk through the proposed file layout and the `RobotCommand` /
`RobotState` struct definitions with me before generating large amounts of
code, so we can catch any pin conflicts, naming mismatches against the app
side, or protocol field disagreements early.