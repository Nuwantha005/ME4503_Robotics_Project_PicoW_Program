# AGENT.md

This is the firmware repository for an autonomous wheeled mobile robot
(WMR), built on a Raspberry Pi Pico W. See `project_description.md` for the
overall project context (this repo is one of two — the other is the Android
app that controls it) and `architecture.md` for the full technical design.

## Build & flash

- `./flash.sh` builds and writes the project to the Pico W board.
- Standard Pico SDK CMake project underneath; `flash.sh` wraps the usual
  `mkdir build && cmake .. && make && picotool load` sequence — read it
  before assuming what it does.
- `PICO_BOARD=pico_w` must be set; this is a WiFi-capable build, not a
  generic Pico build.
- Serial output (for debugging) is over USB at `/dev/ttyACM0`.

## Repo layout

```
src/
  main.cpp
  common/      # shared structs, pin config, critical-section helpers, GPIO IRQ dispatcher
  drivers/     # one file per peripheral TYPE (not per instance — e.g. one
               # tof_vl53l0x.cpp handles all 4 ToF sensors, not 4 files)
  core0/       # WiFi, WebSocket server, JSON protocol, HTTP serving
  core1/       # control loop: PID, kinematics, mode dispatch, line follow, obstacle filter
external/
  cjson/       # borrowed header + cxx for JSON parsing — do not reinvent JSON parsing
experimentation/
  # isolated single-feature test projects from the bring-up phase
  # (blink, IMU, Motor_Single, Motor_Single_with_Encoder, TOF_Sensor, IR, temp_sensor)
  # treat these as reference code to port from, not code to import directly —
  # the integrated firmware has its own driver layer in src/drivers/
```

## Rules

- Use modules inside `src/`, and give each top-level module folder
  (`common/`, `drivers/`, `core0/`, `core1/`) its own `CMakeLists.txt`,
  built as a static library and linked into the single top-level
  executable target. Do not put unrelated logic in `main.cpp` — it should
  only wire the modules together and launch core 1.
- One driver file per **peripheral type**, not per physical instance. Four
  ToF sensors are one `tof_vl53l0x.cpp` exposing array-returning functions,
  not four files. Same for the four encoders. The QTR-8RC is already a
  single peripheral (8 channels, one driver file).
- All GPIO pin numbers, I2C addresses, and physical robot constants (wheel
  radius, wheelbase, gear ratio) live in `common/pin_config.h` — never
  hardcode a pin number inside a driver file. This is also where you check
  for pin conflicts before wiring anything new.
- **GPIO interrupts are a single shared resource per core.** Any module
  that needs a GPIO interrupt (currently: IR array, encoders) must register
  through `common/gpio_irq_dispatch.h`, never call
  `gpio_set_irq_enabled_with_callback()` directly — that call installs one
  global callback per core and a second caller will silently overwrite the
  first. See `architecture.md` → "GPIO interrupt dispatch" for the pattern.
- Core 0 owns WiFi/lwIP and nothing else touches it. Core 1 owns every
  physical peripheral (motors, encoders, IMU, ToF, IR) and nothing else
  touches those. The only crossing point between cores is the
  critical-section-protected shared structs in `common/robot_state.h` —
  see `architecture.md` for their layout and the locking pattern.
- Don't hand-roll JSON parsing — use `external/cjson`.
- Don't hand-roll WebSocket framing beyond what's already documented in
  `architecture.md` — the framing code there intentionally skips the
  extended-length (>126 byte) case since our messages don't need it; don't
  add complexity for a case we don't hit.
- Every new sensor/feature should be buildable and testable against a
  matching test screen in the Android app's hamburger-menu test harness
  before it's wired into the main control loop — don't integrate a feature
  you haven't independently verified end-to-end over WiFi.
- Keep ISRs short — timestamp/counter writes only, no math, no blocking
  calls. Anything an ISR touches must be `volatile`.


## Woking
- When a planning task is given, save the entire plan to `.agent/plans` folder.
- After each major change / implementation, log them in `.agent/TASK_LOG.md`.


## Building in a Linux Native Folder
Due to some NTFS reading issues, build folder in NTFS partition `Work` sometimes get stuck during the build process. Therefore, I decided to use `~/.cache/pico_builds/pico_code` as the linux native build folder. `./flash.sh` has been updated to use that, and if manually compile, use following commands in order. Note that we need to symlink the compile commands to vscode for intelliscence, which is a one time operation.

```
mkdir -p ~/.cache/pico_builds/pico_code
cmake -DPICO_BOARD=pico_w -S . -B ~/.cache/pico_builds/pico_code
ln -sf ~/.cache/pico_builds/pico_code/compile_commands.json ./compile_commands.json
cmake --build ~/.cache/pico_builds/pico_code
```

