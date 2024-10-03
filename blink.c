/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include "stu.c"
#include <math.h>
#include <time.h>
#include <stdio.h>
#include "game/TE_Image.h"
#include "hardware/gpio.h"

// Pico W devices use a GPIO on the WIFI chip for the LED,
// so when building for Pico W, CYW43_WL_GPIO_LED_PIN will be defined
#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

#ifndef LED_DELAY_MS
#define LED_DELAY_MS 50
#endif

#define GPIO_LED_R 11
#define GPIO_LED_G 10
#define GPIO_LED_B 12

// Perform initialisation
int pico_led_init(void)
{
    RuntimeContext_init();
    return PICO_OK;
}

// Turn the led on or off
void pico_set_led(bool led_on, bool g, bool b)
{
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

void init(RuntimeContext *ctx);
void update(RuntimeContext *ctx);

void setRGB(uint8_t r, uint8_t g, uint8_t b)
{
    gpio_put(GPIO_LED_R, !r);
    gpio_put(GPIO_LED_G, !g);
    gpio_put(GPIO_LED_B, !b);
}


void HardFault_Handler(void)
{
    // Custom HardFault handler
    printf("HardFault detected!\n");
    while (true) {
        // Blink an LED or take other appropriate action
        gpio_put(GPIO_LED_R, 1);
        sleep_ms(500);
        gpio_put(GPIO_LED_R, 0);
        sleep_ms(500);
    }
}

void OnPanic(const char *msg)
{
    printf("Panic: %s\n", msg);
    while (true) {
        // Blink an LED or take other appropriate action
        gpio_put(GPIO_LED_R, 1);
        sleep_ms(500);
        gpio_put(GPIO_LED_R, 0);
        sleep_ms(500);
    }
}

void OnPrint(const char *tx)
{
    static int count = 0;
    count++;
    setRGB(count & 1, count & 2, count & 4);
    // printf("%s\n", tx);
}

int main()
{
    stdio_init_all();
    int rc = pico_led_init();

    hard_assert(rc == PICO_OK);
    int bits = 0;
    uint32_t prevTime = time_us_32();
    RuntimeContext *ctx = RuntimeContext_update();
    ctx->dbgSetRGB = setRGB;
    ctx->panic = OnPanic;
    ctx->log = OnPrint;
    TE_Img img = (TE_Img) {
        .p2height = 7,
        .p2width = 7,
        .data = ctx->screenData
    };
    TE_Img_clear(&img, 0x000000, 0);
    TE_Img_fillRect(&img, 0, 32, 128, 64, 0xff88ff00, (TE_ImgOpState) {0});
    RuntimeContext_update();

    setRGB(0, 1, 1);
    init(ctx);
    while (true)
    {
        setRGB(0, 0, 0);
        uint32_t currentTime = time_us_32();
        // cap to 60fps
        while (currentTime - prevTime < 16666)
        {
            sleep_us(16666 - (currentTime - prevTime));
            currentTime = time_us_32();
        }
        prevTime = currentTime;

        setRGB(1, 0, 0);
        ctx = RuntimeContext_update();
        ctx->getUTime = time_us_32;

        if (stdio_usb_connected())
        {
            int input = getchar_timeout_us(0);
            static uint8_t isPressed[16];
            if (input != PICO_ERROR_TIMEOUT && input != PICO_ERROR_NO_DATA && input < 250 && input > 0)
            {
                printf("Input: %c / %d\n", input, input);
                switch (input)
                {
                    case 'W':
                        isPressed[0] = 1;
                        break;
                    case 'w':
                        isPressed[0] = 0;
                        break;
                    case 'S':
                        isPressed[1] = 1;
                        break;
                    case 's':
                        isPressed[1] = 0;
                        break;
                    case 'A':
                        isPressed[2] = 1;
                        break;
                    case 'a':
                        isPressed[2] = 0;
                        break;
                    case 'D':
                        isPressed[3] = 1;
                        break;
                    case 'd':   
                        isPressed[3] = 0;
                        break;
                    case 'I':
                        isPressed[4] = 1;
                        break;
                    case 'i':
                        isPressed[4] = 0;
                        break;
                    case 'J':   
                        isPressed[5] = 1;
                        break;
                    case 'j':
                        isPressed[5] = 0;
                        break;
                    case 'Q':
                        isPressed[6] = 1;
                        break;
                    case 'q':
                        isPressed[6] = 0;
                        break;
                    case 'E':
                        isPressed[7] = 1;
                        break;
                    case 'e':
                        isPressed[7] = 0;
                        break;
                    case 'M':
                        isPressed[8] = 1;
                        break;
                    case 'm':
                        isPressed[8] = 0;
                        break;
                }
            }
            ctx->inputUp |= isPressed[0];
            ctx->inputDown |= isPressed[1];
            ctx->inputLeft |= isPressed[2];
            ctx->inputRight |= isPressed[3];
            ctx->inputA |= isPressed[4];
            ctx->inputB |= isPressed[5];
            ctx->inputShoulderLeft |= isPressed[6];
            ctx->inputShoulderRight |= isPressed[7];
            ctx->inputMenu |= isPressed[8];
        }

        ctx->flags = 0;
        // debug output controls to LED for now
        ctx->rgbLightRed = ctx->inputRight;
        ctx->rgbLightGreen = ctx->inputLeft;
        ctx->rgbLightBlue = ctx->inputDown;
        if (ctx->inputUp)
        {
            ctx->rgbLightGreen |= ctx->rgbLightRed = 1;
        }

        setRGB(0, 1, 0);
        update(ctx);
        setRGB(0, 0, 1);
    }
}
