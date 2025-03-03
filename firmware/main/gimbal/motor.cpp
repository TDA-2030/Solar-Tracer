/* LEDC (LED Controller) basic example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "board.h"
#include "motor.h"

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES           LEDC_TIMER_10_BIT
#define LEDC_FREQUENCY          (25000) // Frequency in Hertz.


/* Warning:
 * For ESP32, ESP32S2, ESP32S3, ESP32C3, ESP32C2, ESP32C6, ESP32H2, ESP32P4 targets,
 * when LEDC_DUTY_RES selects the maximum duty resolution (i.e. value equal to SOC_LEDC_TIMER_BIT_WIDTH),
 * 100% duty cycle is not reachable (duty cannot be set to (2 ** SOC_LEDC_TIMER_BIT_WIDTH)).
 */

// 添加静态成员定义
bool Motor::is_init = false;

static void motor_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 4 kHz
        .clk_cfg          = LEDC_AUTO_CLK,
        .deconfigure      = false,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .gpio_num       = 0,
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0,
        .sleep_mode     = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
        .flags          = {
            .output_invert = 0,
        },
    };
    ledc_channel.channel = LEDC_CHANNEL_0;
    ledc_channel.gpio_num = BOARD_IO_MOTX_IN1;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    ledc_channel.channel = LEDC_CHANNEL_1;
    ledc_channel.gpio_num = BOARD_IO_MOTX_IN2;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    ledc_channel.channel = LEDC_CHANNEL_2;
    ledc_channel.gpio_num = BOARD_IO_MOTY_IN1;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    ledc_channel.channel = LEDC_CHANNEL_3;
    ledc_channel.gpio_num = BOARD_IO_MOTY_IN2;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

}

Motor::Motor(uint8_t _mot_id) : mot_id(_mot_id)
{
    if(!is_init){
        motor_init();
        is_init = true;
    }
}

/**
 * @brief Set motor PWM
 * @note
 * @param mot_id motor id
 * @param percent PWM duty cycle, range from -1000 to 1000, 1000 means 100% duty cycle, miuns means reverse direction
 */
void Motor::set_pwm(int32_t percent)
{
    int32_t duty = percent * 1024 / 1000;
    int channel = LEDC_CHANNEL_0 + this->mot_id * 2;
    if(percent < 0)
    {
        duty = -duty;
        ledc_set_duty(LEDC_MODE, (ledc_channel_t)channel, duty);
        ledc_set_duty(LEDC_MODE, (ledc_channel_t)(channel + 1), 1024);
    } else {
        ledc_set_duty(LEDC_MODE, (ledc_channel_t)channel, 1024);
        ledc_set_duty(LEDC_MODE, (ledc_channel_t)(channel + 1), duty);
    }
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, (ledc_channel_t)channel));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, (ledc_channel_t)(channel + 1)));
}



