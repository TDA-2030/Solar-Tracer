/* LEDC (LED Controller) basic example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "board.h"
#include "motor.h"
#include "pid.h"

static const char *TAG = "motor";

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES           LEDC_TIMER_10_BIT
#define LEDC_FREQUENCY          (25000) // Frequency in Hertz.

#define STALL_SPEED_THRESHOLD   0.1f    // Speed threshold for stall detection
#define RECOVERY_OUTPUT_RATIO   0.8f    // Output ratio threshold for recovery

/* Warning:
 * For ESP32, ESP32S2, ESP32S3, ESP32C3, ESP32C2, ESP32C6, ESP32H2, ESP32P4 targets,
 * when LEDC_DUTY_RES selects the maximum duty resolution (i.e. value equal to SOC_LEDC_TIMER_BIT_WIDTH),
 * 100% duty cycle is not reachable (duty cannot be set to (2 ** SOC_LEDC_TIMER_BIT_WIDTH)).
 */

// 添加静态成员定义
bool Motor::is_init = false;

static void encoder_init(pcnt_unit_handle_t *pcnt_unit)
{
    ESP_LOGI(TAG, "install pcnt unit");
    pcnt_unit_config_t unit_config = {
        .low_limit = -3000,
        .high_limit = 3000,
        .intr_priority = 0,
        .flags = {
            .accum_count = true,
        }
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, pcnt_unit));

    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(*pcnt_unit, &filter_config));

    ESP_LOGI(TAG, "install pcnt channels");
    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = BOARD_IO_MOTX_ENC_A,
        .level_gpio_num = BOARD_IO_MOTX_ENC_B,
    };
    pcnt_channel_handle_t pcnt_chan_a = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(*pcnt_unit, &chan_a_config, &pcnt_chan_a));
    pcnt_chan_config_t chan_b_config = {
        .edge_gpio_num = BOARD_IO_MOTX_ENC_B,
        .level_gpio_num = BOARD_IO_MOTX_ENC_A,
    };
    pcnt_channel_handle_t pcnt_chan_b = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(*pcnt_unit, &chan_b_config, &pcnt_chan_b));

    ESP_LOGI(TAG, "set edge and level actions for pcnt channels");
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    ESP_LOGI(TAG, "add watch points and register callbacks");
    int watch_points[] = {unit_config.low_limit, unit_config.high_limit};
    for (size_t i = 0; i < sizeof(watch_points) / sizeof(watch_points[0]); i++) {
        ESP_ERROR_CHECK(pcnt_unit_add_watch_point(*pcnt_unit, watch_points[i]));
    }

    ESP_ERROR_CHECK(pcnt_unit_enable(*pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(*pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(*pcnt_unit));

    // // Report counter value
    // int pulse_count = 0;
    // int event_count = 0;
    // while (1) {

    //     ESP_ERROR_CHECK(pcnt_unit_get_count(*pcnt_unit, &pulse_count));
    //     ESP_LOGI(TAG, "Pulse count: %d", pulse_count);

    // }
}


static void motor_init(void)
{
    ESP_LOGI("MOTOR", "Initializing motor...");
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

Motor::Motor(uint8_t _mot_id, float _cpr) : mot_id(_mot_id), cpr(_cpr)
{
    encoder_init(&pcnt_unit);
    if (!is_init) {
        motor_init();
        is_init = true;
    }
}

/**
 * @brief Set motor PWM
 * @note
 * @param percent PWM duty cycle, range from -1000 to 1000, 1000 means 100% duty cycle, miuns means reverse direction
 */
void Motor::set_pwm(int32_t percent)
{
    percent = percent > 1000 ? 1000 : percent;
    percent = percent < -1000 ? -1000 : percent;
    int32_t duty = percent * 1024 / 1000;
    int channel = LEDC_CHANNEL_0 + this->mot_id * 2;
    if (percent < 0) {
        ledc_set_duty(LEDC_MODE, (ledc_channel_t)channel, 1024 + duty);
        ledc_set_duty(LEDC_MODE, (ledc_channel_t)(channel + 1), 1024);
    } else {
        ledc_set_duty(LEDC_MODE, (ledc_channel_t)channel, 1024);
        ledc_set_duty(LEDC_MODE, (ledc_channel_t)(channel + 1), 1024 - duty);
    }
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, (ledc_channel_t)channel));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, (ledc_channel_t)(channel + 1)));
}

void Motor::get_postion(float *revolutions)
{
    int pulse_count;
    pcnt_unit_get_count(pcnt_unit, &pulse_count);
    *revolutions = (float)pulse_count / cpr;
}

void Motor::get_velocity(float *velocity)
{
    *velocity = current_speed;
}

void Motor::enable(bool is_enable)
{
    if (state != MOT_STATE_IDLE) {
        return;
    }

    state = is_enable ? MOT_STATE_RUNNING : MOT_STATE_IDLE;
}

void Motor::run(float dt)
{
    if (state == MOT_STATE_IDLE) {
        set_pwm(0);
        return;
    }

    float revolutions;
    get_postion(&revolutions);
    current_speed = (revolutions - last_revolutions) / dt;
    last_revolutions = revolutions;

    // 防止dt过小导致速度计算不准确
    if (dt < 0.001f) {
        return;
    }

    // Always calculate PID even in WARNING state
    float _speed = pid_calculate(&positionPID, revolutions, target_position, dt);
    float output = pid_calculate(&velocityPID, current_speed, _speed, dt);

    // 只在DEBUG级别打印调试信息，减少串口开销
    ESP_LOGD(TAG, "pos:%.2f spd:%.2f out:%.2f state:%d", 
             revolutions, current_speed, output, state);

    // Check state and handle accordingly
    switch (state) {
        case MOT_STATE_RUNNING:
            // Check for stall condition with hysteresis
            if (fabs(current_speed) < STALL_SPEED_THRESHOLD && 
                fabs(output) >= velocityPID.param.max_out * 0.95f) {
                state = MOT_STATE_WARNING;
                set_pwm(0);
                ESP_LOGW(TAG, "Motor stalled! pos:%.2f spd:%.2f out:%.2f", 
                        revolutions, current_speed, output);
            } else {
                set_pwm(output);
            }
            break;

        case MOT_STATE_WARNING:
            // Add hysteresis for recovery to prevent oscillation
            if (fabs(output) < velocityPID.param.max_out * RECOVERY_OUTPUT_RATIO) {
                state = MOT_STATE_RUNNING;
                set_pwm(output);
                ESP_LOGI(TAG, "Motor recovered from stall");
            }
            break;

        default:
            set_pwm(0);
            break;
    }
}

void Motor::set_postion(float position)
{
    this->target_position = position;
}
