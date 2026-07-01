add_library(tests_lib STATIC
    led_test.cpp
)

target_include_directories(tests_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(tests_lib PUBLIC
    common_lib
    pico_cyw43_arch_lwip_poll  # needs cyw43_arch_gpio_put for the LED
)
