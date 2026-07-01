add_library(core0_lib STATIC
    wifi_setup.cpp
    http_server.cpp
    websocket.cpp
    json_protocol.cpp
    core0_main.cpp
    dhcpserver.c
)

target_include_directories(core0_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(core0_lib PUBLIC
    common_lib
    tests_lib
    cjson
    pico_cyw43_arch_lwip_poll
    pico_lwip_core
    pico_lwip_core4
    pico_mbedtls              # mbedtls/sha1.h for WebSocket handshake
)
