add_library(core1_lib STATIC
    core1_main.cpp
)

target_include_directories(core1_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(core1_lib PUBLIC
    common_lib
)
