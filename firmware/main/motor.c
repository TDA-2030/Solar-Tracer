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

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          (10) // Define the output GPIO
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_10_BIT
#define LEDC_FREQUENCY          (25000) // Frequency in Hertz.

static void motor_timercb(TimerHandle_t xTimer);
static TimerHandle_t timer = NULL;
static uint32_t motor_speed = 0;

/* Warning:
 * For ESP32, ESP32S2, ESP32S3, ESP32C3, ESP32C2, ESP32C6, ESP32H2, ESP32P4 targets,
 * when LEDC_DUTY_RES selects the maximum duty resolution (i.e. value equal to SOC_LEDC_TIMER_BIT_WIDTH),
 * 100% duty cycle is not reachable (duty cannot be set to (2 ** SOC_LEDC_TIMER_BIT_WIDTH)).
 */

void motor_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 4 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    timer = xTimerCreate("motor_timer", 100 / portTICK_PERIOD_MS, pdFALSE, NULL, motor_timercb);

}

void motor_set_pwm(uint32_t percent)
{
    static uint32_t last_duty = 0;
    uint32_t duty = percent * 1024 / 1000;
    if (duty == last_duty) {
        return; // No need to change
    }
    last_duty = duty;
    // Set duty to 50%
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty));
    // Update duty to apply the new value
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
}

static void motor_timercb(TimerHandle_t xTimer)
{
    static uint32_t motor_out = 0;
    if (motor_out) {
        motor_out = 0;
        motor_set_pwm(0);
        xTimerStop(xTimer, portMAX_DELAY);//首先停止当前的定时器
        xTimerChangePeriod(xTimer, pdMS_TO_TICKS(200), portMAX_DELAY); //更改定时器周期
        xTimerReset(xTimer, portMAX_DELAY);//复位定时器是为了让时间从当前时间开始而不是从之前启动定时器开始
    } else {
        motor_set_pwm((motor_speed*5)+100);
        motor_out = 1;
        xTimerStop(xTimer, portMAX_DELAY);//首先停止当前的定时器
        xTimerChangePeriod(xTimer, pdMS_TO_TICKS(7), portMAX_DELAY); //更改定时器周期
        xTimerReset(xTimer, portMAX_DELAY);//复位定时器是为了让时间从当前时间开始而不是从之前启动定时器开始
    }
    if (motor_speed > 0) {
        xTimerStart(xTimer, portMAX_DELAY); // 重新启动定时器
    }
}

void motor_set_speed(uint32_t speed)
{
    motor_speed = speed;
    if (speed > 0) {
        if (xTimerIsTimerActive(timer) == pdFALSE) {
            xTimerStart(timer, 0);
        }
    } else {
        xTimerStop(timer, 0);
        motor_set_pwm(0);
    }
}
