add_library(common_lib STATIC
    shared_state.cpp
    gpio_irq_dispatch.cpp
)

target_include_directories(common_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(common_lib PUBLIC
    pico_stdlib
    pico_sync       # critical_section_t
    pico_multicore  # multicore_launch_core1 visible to users of this lib
)
