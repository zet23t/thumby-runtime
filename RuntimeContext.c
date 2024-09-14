#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include <pico/types.h>
#include <stdbool.h>
#include <stdint.h>
#include "game/game.h"

#include "engine_display_driver_rp2_gc9107.c"

#define PI      3.14159265358979323846f
#define HALF_PI 1.57079632679489661923f
#define TWICE_PI 6.28318530717958647692f
#define RAD2DEG (180.0f/PI)
#define DEG2RAD (PI/180.0f)
#define EPSILON 1e-6f

#define GPIO_BUTTON_DPAD_UP         1
#define GPIO_BUTTON_DPAD_LEFT       0
#define GPIO_BUTTON_DPAD_DOWN       3
#define GPIO_BUTTON_DPAD_RIGHT      2
#define GPIO_BUTTON_A               21
#define GPIO_BUTTON_B               25
#define GPIO_BUTTON_BUMPER_LEFT     6
#define GPIO_BUTTON_BUMPER_RIGHT    22
#define GPIO_BUTTON_MENU            26
#define GPIO_RUMBLE                 5
#define GPIO_CHARGE_STAT            24
#define GPIO_LED_R                  11
#define GPIO_LED_G                  10
#define GPIO_LED_B                  12

RuntimeContext runtimeContext;

void RuntimeContext_init(){
    //ENGINE_PRINTF("EngineInput: Setting up...\n");

    gpio_init(GPIO_BUTTON_DPAD_UP);
    gpio_init(GPIO_BUTTON_DPAD_LEFT);
    gpio_init(GPIO_BUTTON_DPAD_DOWN);
    gpio_init(GPIO_BUTTON_DPAD_RIGHT);
    gpio_init(GPIO_BUTTON_A);
    gpio_init(GPIO_BUTTON_B);
    gpio_init(GPIO_BUTTON_BUMPER_LEFT);
    gpio_init(GPIO_BUTTON_BUMPER_RIGHT);
    gpio_init(GPIO_BUTTON_MENU);
    gpio_init(GPIO_LED_R);
    gpio_init(GPIO_LED_G);
    gpio_init(GPIO_LED_B);

    gpio_pull_up(GPIO_BUTTON_DPAD_UP);
    gpio_pull_up(GPIO_BUTTON_DPAD_LEFT);
    gpio_pull_up(GPIO_BUTTON_DPAD_DOWN);
    gpio_pull_up(GPIO_BUTTON_DPAD_RIGHT);
    gpio_pull_up(GPIO_BUTTON_A);
    gpio_pull_up(GPIO_BUTTON_B);
    gpio_pull_up(GPIO_BUTTON_BUMPER_LEFT);
    gpio_pull_up(GPIO_BUTTON_BUMPER_RIGHT);
    gpio_pull_up(GPIO_BUTTON_MENU);

    gpio_set_dir(GPIO_BUTTON_DPAD_UP, GPIO_IN);
    gpio_set_dir(GPIO_BUTTON_DPAD_LEFT, GPIO_IN);
    gpio_set_dir(GPIO_BUTTON_DPAD_DOWN, GPIO_IN);
    gpio_set_dir(GPIO_BUTTON_DPAD_RIGHT, GPIO_IN);
    gpio_set_dir(GPIO_BUTTON_A, GPIO_IN);
    gpio_set_dir(GPIO_BUTTON_B, GPIO_IN);
    gpio_set_dir(GPIO_BUTTON_BUMPER_LEFT, GPIO_IN);
    gpio_set_dir(GPIO_BUTTON_BUMPER_RIGHT, GPIO_IN);
    gpio_set_dir(GPIO_BUTTON_MENU, GPIO_IN);

    gpio_set_dir(GPIO_CHARGE_STAT, GPIO_IN);
    gpio_pull_up(GPIO_CHARGE_STAT);

    gpio_set_dir(GPIO_LED_R, GPIO_OUT);
    gpio_set_dir(GPIO_LED_G, GPIO_OUT);
    gpio_set_dir(GPIO_LED_B, GPIO_OUT);

    uint rumple_pwm_pin_slice = pwm_gpio_to_slice_num(GPIO_RUMBLE);
    gpio_set_function(GPIO_RUMBLE, GPIO_FUNC_PWM);
    pwm_config rumble_pwm_pin_config = pwm_get_default_config();
    pwm_config_set_clkdiv_int(&rumble_pwm_pin_config, 1);
    pwm_config_set_wrap(&rumble_pwm_pin_config, 2048);   // 125MHz / 1024 = 122kHz
    pwm_init(rumple_pwm_pin_slice, &rumble_pwm_pin_config, true);
    pwm_set_gpio_level(GPIO_RUMBLE, 0);

    engine_display_gc9107_init();
}


float engine_math_map(float value, float in_min, float in_max, float out_min, float out_max){
    float in_range = in_max - in_min;
    if(in_range == 0.0f){
        return (out_max - out_min) / 2.0f;
    }
    return (value - in_min) * (out_max - out_min) / in_range + out_min;
}

float engine_math_map_clamp(float value, float in_min, float in_max, float out_min, float out_max){
    if(value <= in_min){
        return out_min;
    }
    if(value >= in_max){
        return out_max;
    }
    return engine_math_map(value, in_min, in_max, out_min, out_max);
}

static uint16_t buffer[128 * 128];
static void Runtime_upload_rgba8_rgb16()
{
    // converting 8-bit RGBA to 5-6-5 RGB
    for (int i = 0; i < 128 * 128; i++)
    {
        uint32_t abgr = runtimeContext.screenData[i];
        uint8_t r5 = (abgr & 0x00FF0000) >> 19;
        uint8_t g6 = (abgr & 0x0000FF00) >> 10;
        uint8_t b5 = (abgr & 0x000000FF) >> 3;
        buffer[i] = (b5 << 11) | (g6 << 5) | r5;
    }
    engine_display_gc9107_update(buffer);
}

RuntimeContext* RuntimeContext_update(){
    float current_time = time_us_32() / 1000000.0f;
    if (runtimeContext.frameCount > 0)
    {
        runtimeContext.deltaTime = current_time - runtimeContext.time;
    }
    runtimeContext.time = current_time;
    runtimeContext.frameCount++;
    
    runtimeContext.previousInputState = runtimeContext.inputState;

    runtimeContext.inputUp = gpio_get(GPIO_BUTTON_DPAD_UP) == false;
    runtimeContext.inputDown = gpio_get(GPIO_BUTTON_DPAD_DOWN) == false;
    runtimeContext.inputLeft = gpio_get(GPIO_BUTTON_DPAD_LEFT) == false;
    runtimeContext.inputRight = gpio_get(GPIO_BUTTON_DPAD_RIGHT) == false;
    
    runtimeContext.inputA = gpio_get(GPIO_BUTTON_A) == false;
    runtimeContext.inputB = gpio_get(GPIO_BUTTON_B) == false;
    
    runtimeContext.inputShoulderLeft = gpio_get(GPIO_BUTTON_BUMPER_LEFT) == false;
    runtimeContext.inputShoulderRight = gpio_get(GPIO_BUTTON_BUMPER_RIGHT) == false;
    
    runtimeContext.inputMenu = gpio_get(GPIO_BUTTON_MENU) == false;

    gpio_put(GPIO_LED_R, !runtimeContext.rgbLightRed);
    gpio_put(GPIO_LED_G, !runtimeContext.rgbLightGreen);
    gpio_put(GPIO_LED_B, !runtimeContext.rgbLightBlue);

    if(runtimeContext.rumbleIntensity <= EPSILON){
        pwm_set_gpio_level(GPIO_RUMBLE, 0);
    }else{
        pwm_set_gpio_level(GPIO_RUMBLE, (uint32_t)engine_math_map_clamp(runtimeContext.rumbleIntensity, 0.0f, 1.0f, 1400.0f, 2048.0f));
    }

    Runtime_upload_rgba8_rgb16();
    return &runtimeContext;
}


void engine_io_rp3_rumble(float intensity){
    if(intensity <= EPSILON){
        pwm_set_gpio_level(GPIO_RUMBLE, 0);
    }else{
        pwm_set_gpio_level(GPIO_RUMBLE, (uint32_t)engine_math_map_clamp(intensity, 0.0f, 1.0f, 1400.0f, 2048.0f));
    }
}


bool engine_io_rp3_is_charging(){
    // Charge status line from LiIon charger IC is set
    // pulled HIGH by default and is pulled LOW by IC
    return gpio_get(GPIO_CHARGE_STAT);
}