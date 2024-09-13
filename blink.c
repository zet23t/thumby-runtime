/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"

// Pico W devices use a GPIO on the WIFI chip for the LED,
// so when building for Pico W, CYW43_WL_GPIO_LED_PIN will be defined
#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

#ifndef LED_DELAY_MS
#define LED_DELAY_MS 50
#endif

#define GPIO_LED_R                  11
#define GPIO_LED_G                  10
#define GPIO_LED_B                  12

// Perform initialisation
int pico_led_init(void) {
#if defined(GPIO_LED_R)
    // A device like Pico that uses a GPIO for the LED will define GPIO_LED_R
    // so we can use normal GPIO functionality to turn the led on and off
    gpio_init(GPIO_LED_R);
    gpio_set_dir(GPIO_LED_R, GPIO_OUT);
    gpio_init(GPIO_LED_G);
    gpio_set_dir(GPIO_LED_G, GPIO_OUT);
    gpio_init(GPIO_LED_B);
    gpio_set_dir(GPIO_LED_B, GPIO_OUT);
    return PICO_OK;
#elif defined(CYW43_WL_GPIO_LED_PIN)
    // For Pico W devices we need to initialise the driver etc
    return cyw43_arch_init();
#endif
}

// Turn the led on or off
void pico_set_led(bool led_on, bool g, bool b) {
#if defined(GPIO_LED_R)
    // Just set the GPIO on or off
    gpio_put(GPIO_LED_R, led_on);
    gpio_put(GPIO_LED_G, g);
    gpio_put(GPIO_LED_B, b);
#elif defined(CYW43_WL_GPIO_LED_PIN)
    // Ask the wifi "driver" to set the GPIO on or off
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
#endif
}

int main() {
    int rc = pico_led_init();
    hard_assert(rc == PICO_OK);
    int bits = 0;
    while (true) {
        bits = (bits + 1) % 8;
        pico_set_led(bits & 1, bits & 2, bits & 4);
        sleep_ms(LED_DELAY_MS);
    }
}
