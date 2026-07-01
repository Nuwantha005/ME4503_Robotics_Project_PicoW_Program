cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

if (NOT DEFINED PICO_BOARD)
    set(PICO_BOARD pico_w CACHE STRING "Board type")
endif()

project(Robotics_Project_Pico_Program C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

pico_sdk_init()

# ── External libraries ──────────────────────────────────────────────────────
add_subdirectory(external/cjson)

# ── Source modules (each is a static library) ───────────────────────────────
add_subdirectory(src/common)
add_subdirectory(src/tests)
add_subdirectory(src/core1)
add_subdirectory(src/core0)

# ── Main executable ─────────────────────────────────────────────────────────
add_executable(Robotics_Project_Pico_Program
    src/main.cpp
)

target_link_libraries(Robotics_Project_Pico_Program
    common_lib
    tests_lib
    core0_lib
    core1_lib
    cjson
)

# ── USB serial for debugging on /dev/ttyACM0 ────────────────────────────────
pico_enable_stdio_usb(Robotics_Project_Pico_Program 1)
pico_enable_stdio_uart(Robotics_Project_Pico_Program 0)

# ── Generates .uf2, .bin, .hex etc. ─────────────────────────────────────────
pico_add_extra_outputs(Robotics_Project_Pico_Program)
