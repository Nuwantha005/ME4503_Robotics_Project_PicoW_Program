#include "led_test.h"
#include "pico/cyw43_arch.h"
#include "pin_config.h"
#include <stdio.h>

// ============================================================================
// led_test.cpp  (src/tests/)
//
// Toggles the Pico W onboard LED via cyw43_arch_gpio_put().
// Must only ever be called from Core 0 — the CYW43 SPI bus is Core 0's domain.
// ============================================================================

static bool s_led_state = false;

void led_test_init() {
    s_led_state = false;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    printf("[led_test] Initialised — LED off\n");
}

bool led_test_toggle() {
    s_led_state = !s_led_state;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, s_led_state ? 1 : 0);
    printf("[led_test] LED %s\n", s_led_state ? "ON" : "OFF");
    return s_led_state;
}

bool led_test_get_state() {
    return s_led_state;
}
