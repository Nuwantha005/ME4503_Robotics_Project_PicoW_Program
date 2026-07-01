# Phase 1: Project Skeleton + WiFi / WebSocket / JSON

## Goal

Establish the complete firmware architecture and prove the core networking pipeline:
**Android app button → WebSocket → Core 0 → LED toggle → Core 0 → WebSocket → app display.**

No motors, sensors, PID, or kinematics in this phase — just the communication backbone with LED on/off as the simplest possible payload. Per user feedback, the LED test will be handled entirely on Core 0, and tests will be isolated into a dedicated `tests/` module.

---

## 1. Approved Architecture & Decisions

Based on user feedback, the following designs are approved and will be implemented:
- **CMake Restructure**: Break the flat layout into per-module static libraries (`common`, `core0`, `core1`, `tests`, `cjson`), linked into the main executable.
- **WiFi Poll Mode (`cyw43_arch_lwip_poll`)**: Approved. Core 0 will run a non-blocking poll loop.
- **WebSocket Hand-rolled Framing**: Acknowledged. We will implement the basic WS framing logic over raw lwIP TCP.
- **SHA-1 Dependency**: We will use Pico SDK's bundled `mbedtls/sha1.h` for the WebSocket handshake.
- **WiFi Credentials & Protocols**: The Android app will be built to match the firmware's default IP (`192.168.4.1`), credentials (`WMR_Robot`/`robotwmr123`), and JSON test schemas.
- **Minimal Telemetry**: Phase 1 telemetry will broadcast `uptime_ms` only. Full structs will be defined but largely zeroed.
- **Port 80 Reconnection**: We will proceed with the single port 80 design, managing stale connections by closing the old client when a new one connects.

---

## 2. Proposed File Layout

```
pico_code/
├── CMakeLists.txt              ← [MODIFY] restructure to per-module libraries
├── flash.sh                    ← [KEEP]
├── external/
│   └── cjson/
│       ├── cJSON.c             ← [KEEP]
│       ├── cJSON.h             ← [KEEP]
│       └── CMakeLists.txt      ← [NEW] builds cjson as a static lib
├── src/
│   ├── main.cpp                ← [MODIFY] init cores, call test initializers
│   ├── common/
│   │   ├── CMakeLists.txt      ← [NEW]
│   │   ├── pin_config.h        ← [NEW] LED pin, WiFi config constants
│   │   ├── robot_state.h       ← [NEW] RobotCommand, RobotConfig, RobotState, Waypoint
│   │   ├── shared_state.h      ← [NEW] critical section globals + read/write snapshot fns
│   │   ├── shared_state.cpp    ← [NEW]
│   │   ├── gpio_irq_dispatch.h ← [NEW] stub — not needed until Phase 2
│   │   └── gpio_irq_dispatch.cpp ← [NEW] stub
│   ├── core0/
│   │   ├── CMakeLists.txt      ← [NEW]
│   │   ├── wifi_setup.h        ← [NEW] AP init
│   │   ├── wifi_setup.cpp      ← [NEW]
│   │   ├── http_server.h       ← [NEW] TCP listener, HTTP/WS routing
│   │   ├── http_server.cpp     ← [NEW]
│   │   ├── websocket.h         ← [NEW] WS handshake, frame encode/decode
│   │   ├── websocket.cpp       ← [NEW]
│   │   ├── json_protocol.h     ← [NEW] JSON ↔ struct conversion
│   │   ├── json_protocol.cpp   ← [NEW]
│   │   └── core0_main.cpp      ← [NEW] Core 0 main loop (poll + telemetry timer)
│   ├── core1/
│   │   ├── CMakeLists.txt      ← [NEW]
│   │   └── core1_main.cpp      ← [NEW] 100Hz tick skeleton + heartbeat
│   └── tests/
│       ├── CMakeLists.txt      ← [NEW]
│       ├── led_test.h          ← [NEW] LED toggle logic isolated here
│       └── led_test.cpp        ← [NEW]
```

---

## 3. Proposed Changes — Detail

### 3.1 External: cJSON library
- **CMakeLists.txt**: Builds `cJSON.c` as a static library target `cjson`.

### 3.2 Common module (`src/common/`)
- **`pin_config.h`**: WiFi AP config constants and Pico W LED pin (`CYW43_WL_GPIO_LED_PIN`). Placeholders for future sensors.
- **`robot_state.h`**: Exact struct definitions from `architecture.md`.
- **`shared_state.h / .cpp`**: `critical_section_t` locks and snapshot functions following the snapshot-copy pattern.
- **`gpio_irq_dispatch`**: Stub implementation to be filled in Phase 2.

### 3.3 Core 0 module (`src/core0/`)
- **`wifi_setup`**: Initializes CYW43 in poll mode and configures the AP.
- **`http_server`**: Raw lwIP TCP server. Upgrades to WS or serves a minimal static status page.
- **`websocket`**: Handles the SHA-1 handshake and unmasks/masks text frames.
- **`json_protocol`**: 
  - Routes `"test"` → `"led"` messages to the `tests` module (calls `led_test_toggle()`).
  - Converts `"test_result"` to JSON to send back.
  - Formats `{"type":"telemetry","uptime_ms":...}` broadcasts.
- **`core0_main`**: Runs the non-blocking `cyw43_arch_poll()` loop and the 10 Hz telemetry timer.

### 3.4 Core 1 module (`src/core1/`)
- **`core1_main`**: A simple 100 Hz loop that reads shared state and writes `uptime_ms`. It will **not** process the LED toggle, reserving Core 1's true hardware integration for when motors/sensors are introduced.

### 3.5 Tests module (`src/tests/`)
Per user feedback, test harness logic is isolated here so it doesn't clutter the main control loops.
- **`led_test.h` / `led_test.cpp`**: 
  - Exposes `void led_test_init()` to be called from `main.cpp`.
  - Exposes `bool led_test_toggle()` to toggle the CYW43 LED and return the new state. This function is called by `json_protocol.cpp` on Core 0 when a test message arrives.

### 3.6 Root files
- **`main.cpp`**: 
  - Wires initialization calls (`stdio_init_all`, `wifi_init`, `http_server_init`, `led_test_init`).
  - Launches Core 1.
  - Enters Core 0 main loop.
- **`CMakeLists.txt`**: Links all new static libraries (`common_lib`, `core0_lib`, `core1_lib`, `tests_lib`, `cjson`).

---

## 4. Verification Plan

### Build Verification
```bash
./flash.sh   # Must compile cleanly and produce a .uf2 file
```

### End-to-End WiFi Verification
1. Connect laptop/phone to the `WMR_Robot` WiFi network.
2. Open `http://192.168.4.1` in a browser to see the minimal status page.
3. Connect a WebSocket tool to `ws://192.168.4.1/`.
4. Send: `{"type":"test","target":"led","action":"toggle"}`
5. Verify response: `{"type":"test_result","target":"led","state":"on"}`
6. Verify Pico W onboard LED toggles (done purely by Core 0).
7. Verify 10 Hz telemetry: `{"type":"telemetry","uptime_ms":...}`

### Android App Verification
- From the Android app's "WiFi connection test" screen:
  1. Tap the toggle button → LED toggles.
  2. Debug text field displays `test_result` JSON and incrementing `uptime_ms`.

---

## 5. Implementation Order

1. **CMake structure**: Setup library targets (`common`, `core0`, `core1`, `tests`, `cjson`).
2. **`common/`**: Define structs, shared state locking, and pin config.
3. **`tests/`**: Implement `led_test`.
4. **`core1/`**: Basic 100 Hz tick loop reporting uptime.
5. **`core0/`**: AP initialization, HTTP TCP listener, WS handshake and framing.
6. **`core0/`**: JSON parsing and wiring `"test"` messages to `led_test_toggle()`.
7. **`main.cpp`**: Final integration and boot.
