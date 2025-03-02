/*  LED Lightbulb demo implementation using RGB LED

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <sdkconfig.h>
#include <esp_log.h>
#include <iot_button.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_standard_params.h>

#include <app_reset.h>
#include "motor.h"
#include "ws2812_led.h"

extern esp_rmaker_device_t *light_device;


/* This is the button that is used for toggling the power */
#define BUTTON_GPIO          CONFIG_EXAMPLE_BOARD_BUTTON_GPIO
#define BUTTON_ACTIVE_LEVEL  0

#define WIFI_RESET_BUTTON_TIMEOUT       3
#define FACTORY_RESET_BUTTON_TIMEOUT    10

static void _setmode_with_update(LightMode mode)
{
    if (mode != ws2812_effect_get_mode()) {
        if (mode == LIGHT_MODE_NORMAL) {
            ws2812_play_mode_enable(0, 0, 0);
        }
        ws2812_effect_set_mode(mode);
        esp_rmaker_param_update_and_report(
            esp_rmaker_device_get_param_by_type(light_device, ESP_RMAKER_PARAM_MODE),
            esp_rmaker_str(ws2812_effect_get_mode_name()));
    }
}


esp_err_t app_light_set_led(uint32_t hue, uint32_t saturation, uint32_t brightness)
{
    /* Whenever this function is called, light power will be ON */
    if (!ws2812_get_power()) {
        esp_rmaker_param_update_and_report(
            esp_rmaker_device_get_param_by_type(light_device, ESP_RMAKER_PARAM_POWER),
            esp_rmaker_bool(ws2812_get_power()));
    }
    _setmode_with_update(LIGHT_MODE_NORMAL);

    return ws2812_led_set_hsv(hue, saturation, brightness);
}

esp_err_t app_light_set_power(bool power)
{
    _setmode_with_update(LIGHT_MODE_NORMAL);
    ws2812_set_power(power);
    return ESP_OK;
}

esp_err_t app_light_init(void)
{
    esp_err_t err = ws2812_led_init();
    if (err != ESP_OK) {
        return err;
    }
    return ESP_OK;
}

esp_err_t app_light_set_brightness(uint16_t brightness)
{
    _setmode_with_update(LIGHT_MODE_NORMAL);
    ws2812_set_brightness(brightness);
    return ESP_OK;
}

esp_err_t app_light_set_hue(uint16_t hue)
{
    _setmode_with_update(LIGHT_MODE_NORMAL);
    ws2812_set_hue(hue);
    return ESP_OK;
}

esp_err_t app_light_set_saturation(uint16_t saturation)
{
    _setmode_with_update(LIGHT_MODE_NORMAL);
    ws2812_set_saturation(saturation);
    return ESP_OK;
}

esp_err_t app_motor_set_speed(uint32_t speed)
{
    motor_set_speed(speed);
    return ESP_OK;
}

esp_err_t app_light_set_mode(const char *mode)
{
    ws2812_effect_set_mode_name(mode);
    return ESP_OK;
}

esp_err_t app_light_set_text(const char *text)
{
    ws2812_effect_set_text(text);
    return ESP_OK;
}

static void push_btn_cb(void *arg)
{
    _setmode_with_update(LIGHT_MODE_NORMAL);
    app_light_set_power(!ws2812_get_power());
    esp_rmaker_param_update_and_report(
        esp_rmaker_device_get_param_by_type(light_device, ESP_RMAKER_PARAM_POWER),
        esp_rmaker_bool(ws2812_get_power()));
}

void app_driver_init()
{
    motor_init();
    app_light_init();
    button_handle_t btn_handle = iot_button_create(BUTTON_GPIO, BUTTON_ACTIVE_LEVEL);
    if (btn_handle) {
        /* Register a callback for a button tap (short press) event */
        iot_button_set_evt_cb(btn_handle, BUTTON_CB_TAP, push_btn_cb, NULL);
        /* Register Wi-Fi reset and factory reset functionality on same button */
        app_reset_button_register(btn_handle, WIFI_RESET_BUTTON_TIMEOUT, FACTORY_RESET_BUTTON_TIMEOUT);
    }
}
