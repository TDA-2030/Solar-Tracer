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
#include "esp_timer.h"
#include "esp_err.h"
#include "esp_log.h"
#include "board.h"
#include "motor.h"
#include "pid.h"
#include "led.h"

static const char *TAG = "motor";



#define STALL_SPEED_THRESHOLD   0.1f    // Speed threshold for stall detection
#define RECOVERY_OUTPUT_RATIO   0.8f    // Output ratio threshold for recovery


bool EncoderSensor::init(int gpio_enca, int gpio_encb, float cpr)
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
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));

    ESP_LOGI(TAG, "install pcnt channels");
    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = gpio_enca,
        .level_gpio_num = gpio_encb,
    };
    pcnt_channel_handle_t pcnt_chan_a = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));
    pcnt_chan_config_t chan_b_config = {
        .edge_gpio_num = gpio_encb,
        .level_gpio_num = gpio_enca,
    };
    pcnt_channel_handle_t pcnt_chan_b = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_b_config, &pcnt_chan_b));

    ESP_LOGI(TAG, "set edge and level actions for pcnt channels");
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    ESP_LOGI(TAG, "add watch points and register callbacks");
    int watch_points[] = {unit_config.low_limit, unit_config.high_limit};
    for (size_t i = 0; i < sizeof(watch_points) / sizeof(watch_points[0]); i++) {
        ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, watch_points[i]));
    }

    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));
    counts_per_rev = cpr;
    return 0;
}

float EncoderSensor::get_position()
{
    return current_revolutions;
}
float EncoderSensor::get_velocity()
{
    return cur_velocity;
}

void EncoderSensor::update_velocity(float dt)
{
    int pulse_count;
    pcnt_unit_get_count(pcnt_unit, &pulse_count);
    current_revolutions = (float)pulse_count / counts_per_rev;
    cur_velocity = (current_revolutions - last_revolutions) / dt;
    last_revolutions = current_revolutions;
}

void EncoderSensor::clear_position()
{
    pcnt_unit_clear_count(pcnt_unit);
}

/* ======================================================= */

bool IMUMotSensor::init(IMUBase *_imu)
{
    imu = _imu;
    return 0;
}

float IMUMotSensor::get_position()
{
    return current_revolutions;
}
float IMUMotSensor::get_velocity()
{
    return cur_velocity;
}

void IMUMotSensor::update_velocity(float dt)
{
    const imu_data_t &data = imu->getData();
    current_revolutions = data.angle.y;
    cur_velocity = data.gyro.y;
}

void IMUMotSensor::clear_position()
{
}

/* Warning:
 * For ESP32, ESP32S2, ESP32S3, ESP32C3, ESP32C2, ESP32C6, ESP32H2, ESP32P4 targets,
 * when LEDC_DUTY_RES selects the maximum duty resolution (i.e. value equal to SOC_LEDC_TIMER_BIT_WIDTH),
 * 100% duty cycle is not reachable (duty cannot be set to (2 ** SOC_LEDC_TIMER_BIT_WIDTH)).
 */
static int g_pwm_count = 0;
bool PWM::init(int pin1, int pin2, int freq_hz)
{
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES           LEDC_TIMER_10_BIT

    mot_id = g_pwm_count++;

    ESP_LOGI(TAG, "Initializing motor...");
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = (uint32_t)freq_hz,
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

    int channel = LEDC_CHANNEL_0 + mot_id * 2;

    ledc_channel.channel = (ledc_channel_t)channel;
    ledc_channel.gpio_num = pin1;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    ledc_channel.channel = (ledc_channel_t)(channel + 1);
    ledc_channel.gpio_num = pin2;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    return 0;
}

/**
 * @brief Set motor PWM
 * @note
 * @param percent PWM duty cycle, range from -1000 to 1000, 1000 means 100% duty cycle, miuns means reverse direction
 */
void PWM::set_pwm(int32_t percent)
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

Motor::Motor(const char *name)
{
    state = MOT_STATE_IDLE;
    max_speed = 100;
    stall_cnt = 0;
    this->name = name;
}

void Motor::attach_sensor(MotorSensor *sensor)
{
    if (sensor) {
        this->sensor = sensor;
    }
}

void Motor::attach_driver(PWM *pwm)
{
    if (pwm) {
        this->pwm = pwm;
    }
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
        pwm->set_pwm(0);
        return;
    }

    // 防止dt过小导致速度计算不准确
    if (dt < 0.001f) {
        return;
    }

    sensor->update_velocity(dt);
    float current_speed = sensor->get_velocity();
    float revolutions = sensor->get_position();
    float output;
    // Always calculate PID even in WARNING state
    if(name[0] == 'p') {
        output = pid_calculate(&velocityPID, revolutions, target_position, dt);
    } else {
        float _speed = pid_calculate(&positionPID, revolutions, target_position, dt);
        abs_limit(&_speed, max_speed, -max_speed);
        output = pid_calculate(&velocityPID, current_speed, _speed, dt);
    }
    // printf("pos:%.3f,%.3f,%.2f,%.2f,%.2f,%d\n", revolutions, target_position, current_speed, output, max_speed, state);
    // ESP_LOGI(TAG, "id:%d, pos:%.3f, tar:%.3f spd:%.2f out:%.2f state:%d",
    //  revolutions, target_position, current_speed, output, state);

    // Check state and handle accordingly
    switch (state) {
    case MOT_STATE_RUNNING:
        // Check for stall condition with hysteresis
        if (fabs(current_speed) < STALL_SPEED_THRESHOLD &&
                fabs(output) >= velocityPID.param->max_out * 0.95f) {
            if ((++ stall_cnt) > 10) {
                stall_cnt = 0;
                state = MOT_STATE_WARNING;
                led_start_state(LED_RED, BLINK_DOUBLE);
                pwm->set_pwm(0);
                ESP_LOGW(TAG, "Motor%s stalled! pos:%.2f spd:%.2f out:%.2f",
                         name, revolutions, current_speed, output);
            } else {
                pwm->set_pwm(output);
            }
        } else {
            stall_cnt = 0;
            pwm->set_pwm(output);
        }
        break;

    case MOT_STATE_WARNING:
        // Add hysteresis for recovery to prevent oscillation
        if (fabs(output) < velocityPID.param->max_out * RECOVERY_OUTPUT_RATIO) {
            state = MOT_STATE_RUNNING;
            led_stop_state(LED_RED, BLINK_DOUBLE);
            pwm->set_pwm(output);
            ESP_LOGI(TAG, "Motor%s recovered from stall", name);
        }
        break;

    default:
        pwm->set_pwm(0);
        break;
    }
}
