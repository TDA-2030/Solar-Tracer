/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once
#include <esp_err.h>
#include "color.h"

typedef enum {
    LIGHT_MODE_NORMAL,       // 灯板常亮模式
    LIGHT_MODE_RAINBOW,      // 灯板彩虹模式
    LIGHT_MODE_BREATH,       // 呼吸灯模式
    LIGHT_MODE_SPIRAL,  // 色彩擦拭特效
    LIGHT_MODE_WIPE,  // 色彩擦拭特效
    LIGHT_MODE_JUMP,  // 色彩跳跃特效
    LIGHT_MODE_SHIFT, // 色彩偏移特效
    LIGHT_MODE_MAX,
} LightMode;

/** Initialize the WS2812 RGB LED
 *
 * @return ESP_OK on success.
 * @return error in case of failure.
 */
esp_err_t ws2812_led_init(void);

/** Set RGB value for the WS2812 LED
 *
 * @param[in] red Intensity of Red color (0-100)
 * @param[in] green Intensity of Green color (0-100)
 * @param[in] blue Intensity of Green color (0-100)
 *
 * @return ESP_OK on success.
 * @return error in case of failure.
 */
esp_err_t ws2812_led_set_rgb(uint8_t red, uint8_t green, uint8_t blue);

/** Set HSV value for the WS2812 LED
 *
 * @param[in] hue Value of hue in arc degrees (0-360)
 * @param[in] saturation Saturation in percentage (0-100)
 * @param[in] value Value (also called Intensity) in percentage (0-100)
 *
 * @return ESP_OK on success.
 * @return error in case of failure.
 */
esp_err_t ws2812_led_set_hsv(uint32_t hue, uint32_t saturation, uint32_t value);

/** Clear (turn off) the WS2812 LED
 * @return ESP_OK on success.
 * @return error in case of failure.
 */
esp_err_t ws2812_led_clear(void);

esp_err_t ws2812_refresh(void);

void ws2812_write_pixel_xy(uint16_t _x, uint16_t _y, color32_t color);


void ws2812_set_power(bool power);
void ws2812_set_brightness(uint8_t brightness);
void ws2812_set_hue(uint16_t hue);
void ws2812_set_saturation(uint8_t saturation);

void ws2812_effect_set_mode(LightMode mode);
LightMode ws2812_effect_get_mode(void);
void ws2812_effect_set_mode_name(const char* mode);
const char *ws2812_effect_get_mode_name();
const char **ws2812_effect_get_mode_name_array(uint8_t *length);
void ws2812_effect_set_text(const char* text);
const char *ws2812_effect_get_text(void);
void ws2812_play_run_task(void *args);
void ws2812_play_mode_enable(bool enable, uint32_t start_delay, bool exit_to_normal_mode);


bool ws2812_get_power(void);

uint8_t ws2812_get_brightness(void);

uint8_t ws2812_get_hue(void);

uint8_t ws2812_get_saturation(void);

int32_t _random_range(int32_t min, int32_t max);
