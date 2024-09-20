/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "stu.c"
#include <math.h>
#include <time.h>

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
    RuntimeContext_init();
    return PICO_OK;
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

#include "game/TE_Image.h"
#include "game/TE_Font.h"
#include "game/game_assets.h"

void init();
void update(RuntimeContext *ctx);

int main() {
    int rc = pico_led_init();
    hard_assert(rc == PICO_OK);
    int bits = 0;
    uint32_t prevTime = time_us_32();
    
    init();
    while (true) {
        uint32_t currentTime = time_us_32();
        //cap to 60fps
        while (currentTime - prevTime < 16666) {
            sleep_us(16666 - (currentTime - prevTime));
            currentTime = time_us_32();
        }
        prevTime = currentTime;
        
        RuntimeContext *ctx = RuntimeContext_update();
        ctx->getUTime = time_us_32;
        
        ctx->flags = 0;
        // debug output controls to LED for now
        ctx->rgbLightRed = ctx->inputRight;
        ctx->rgbLightGreen = ctx->inputLeft;
        ctx->rgbLightBlue = ctx->inputDown;
        if (ctx->inputUp) {
                ctx->rgbLightGreen |= ctx->rgbLightRed = 1;
        }

        update(ctx);

    }
}
