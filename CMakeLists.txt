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

add_executable(Robotics_Project_Pico_Program 
    main.cpp
    )

# Pico W needs cyw43_arch for the LED
target_link_libraries(Robotics_Project_Pico_Program
    pico_stdlib
    hardware_pwm
    hardware_gpio
    #cyw43_arch
)

# Creates .uf2 file for drag-drop flashing
pico_enable_stdio_usb(Robotics_Project_Pico_Program 1)
pico_enable_stdio_uart(Robotics_Project_Pico_Program 0)

pico_add_extra_outputs(Robotics_Project_Pico_Program)
