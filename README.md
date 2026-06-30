# Pico Project Template (Build + Flash + VS Code)

This project can be reused as a template for new Raspberry Pi Pico or Pico W experiments.

## What this template already gives you

- CMake-based Pico SDK project setup
- `flash.sh` script to configure (if needed), build, and flash `.uf2`
- VS Code integration:
  - `Ctrl+Shift+B` runs build+flash task
  - `F5` runs the same flow through launch config
- IntelliSense setup using `build/compile_commands.json`

## Files to copy into a new project

Copy these from this folder into your new project folder:

- `CMakeLists.txt`
- `flash.sh`
- `.vscode/settings.json`
- `.vscode/c_cpp_properties.json`
- `.vscode/tasks.json`
- `.vscode/launch.json`

Do not copy:

- `build/`

Then add your own source files.

## Minimum steps for a new C project

1. Create new folder for your project, for example `temp_sensor`.
2. Copy the template files listed above.
3. Add your main source file, for example `temp_sensor.c`.
4. Update `CMakeLists.txt`:
   - `project(blink C CXX ASM)` -> `project(temp_sensor C CXX ASM)`
   - `add_executable(blink blink.c)` -> `add_executable(temp_sensor temp_sensor.c)`
   - Replace all target references `blink` with `temp_sensor` in:
     - `target_link_libraries(...)`
     - `pico_enable_stdio_usb(...)`
     - `pico_enable_stdio_uart(...)`
     - `pico_add_extra_outputs(...)`
5. Open the new folder in VS Code.
6. Run `./flash.sh` or use `F5` / `Ctrl+Shift+B`.

## Board selection (Pico vs Pico W)

This template currently defaults to Pico W:

- In `CMakeLists.txt`:
  - `set(PICO_BOARD pico_w CACHE STRING "Board type")`
- In `.vscode/settings.json`:
  - `"PICO_BOARD": "pico_w"`

For normal Pico (non-W), change both to `pico`.

## When to keep or remove CYW43

`pico/cyw43_arch.h` and `pico_cyw43_arch_none` are for Pico W features such as Wi-Fi chip LED.

Keep CYW43 if:

- You are targeting Pico W and using CYW43 APIs

Remove CYW43 if:

- You are using normal Pico
- You are not using CYW43 APIs (for example ADC temp sensor only)

If removing CYW43:

1. Remove `#include "pico/cyw43_arch.h"` and related function calls from code.
2. Remove `pico_cyw43_arch_none` from `target_link_libraries(...)`.

## C++ project variant

You can use the same template for C++.

### File changes

- Rename main file to `main.cpp` (or another `.cpp` file)
- Keep C files too if needed; mixed C/C++ builds are supported

### CMake changes

Use a C++ target source list, example:

```cmake
add_executable(temp_sensor_cpp
    src/main.cpp
)

target_link_libraries(temp_sensor_cpp
    pico_stdlib
)

pico_enable_stdio_usb(temp_sensor_cpp 1)
pico_enable_stdio_uart(temp_sensor_cpp 0)
pico_add_extra_outputs(temp_sensor_cpp)
```

Notes:

- `set(CMAKE_CXX_STANDARD 17)` is already present in template
- If you include Pico W CYW43 code in C++, also link `pico_cyw43_arch_none`

## Multi-file project layout (`src/` + `modules/`)

A common structure:

```text
my_project/
  CMakeLists.txt
  flash.sh
  .vscode/
  src/
    main.c
    modules/
      temp_sensor.c
      temp_sensor.h
      led.c
      led.h
```

CMake example for a C multi-file target:

```cmake
add_executable(my_project
    src/main.c
    src/modules/temp_sensor.c
    src/modules/led.c
)

target_include_directories(my_project PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src/modules
)

target_link_libraries(my_project
    pico_stdlib
)

pico_enable_stdio_usb(my_project 1)
pico_enable_stdio_uart(my_project 0)
pico_add_extra_outputs(my_project)
```

CMake example with an internal module library:

```cmake
add_library(app_modules
    src/modules/temp_sensor.c
    src/modules/led.c
)

target_include_directories(app_modules PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/src/modules
)

add_executable(my_project src/main.c)

target_link_libraries(my_project
    app_modules
    pico_stdlib
)

pico_enable_stdio_usb(my_project 1)
pico_enable_stdio_uart(my_project 0)
pico_add_extra_outputs(my_project)
```

For C++ modules, the same approach applies using `.cpp` files.

## How `flash.sh` works

`flash.sh` does the following:

1. Moves to script directory
2. Configures CMake if `build/CMakeCache.txt` is missing
3. Builds project with `cmake --build build`
4. Finds generated `.uf2`
5. Reboots Pico into BOOTSEL mode using `picotool`
6. Loads `.uf2`
7. Reboots board

## Typical commands

- Configure manually: `cmake -S . -B build`
- Build manually: `cmake --build build`
- Build+flash script: `./flash.sh`

## VS Code usage

- `F5`: runs launch profile `Pico: Build and Flash (flash.sh)`
- `Ctrl+Shift+B`: runs default task `Pico: Build and Flash`
- Output appears in the integrated VS Code terminal

## Quick checklist when creating a new project

- Rename project name in `project(...)`
- Rename target in `add_executable(...)`
- Update all target-specific CMake calls
- Set correct board (`pico` or `pico_w`) in both CMake and VS Code settings
- Keep/remove CYW43 according to board and code usage
- Verify `flash.sh` is executable: `chmod +x flash.sh`
