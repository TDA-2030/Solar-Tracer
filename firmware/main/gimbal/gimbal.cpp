
#include <memory>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "helper.h"
#include "gimbal.h"
#include "setting.h"
#include "led.h"
#include "adc.h"
#include "board.h"

static const char *TAG = "gimbal";

#define LPF(beta, prev, input) ((beta) * (input) + (1 - (beta)) * (prev))

const char* Gimbal::SysStateDescriptions[] = {
#define X(name, desc) desc,
    SYS_STATE_LIST
#undef X
};

Gimbal::Gimbal(): imu(nullptr), gps(nullptr), pitchMotor(nullptr), yawMotor(nullptr),
    pitchTarget(0), yawTarget(0)

{
    ESP_LOGI(TAG, "Gimbal created");
}

Gimbal::~Gimbal()
{

}

void Gimbal::check_home(float homing_speed)
{
    // check yaw motor home
    float pos_max, pos_min;
    float old_speed = this->yawMotor->get_max_speed();
    this->yawMotor->set_max_speed(homing_speed);
    this->yawMotor->set_position(2 * 360); // set to 360 to make sure it is reach positive limit
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(50));
        if (MOT_STATE_WARNING == this->yawMotor->get_state()) {
            vTaskDelay(pdMS_TO_TICKS(100));
            pos_max = this->yawMotor->get_position(); // get positive limit
            break;
        }
    }
    this->yawMotor->set_position(-2 * 360); // set to -360 to make sure it is reach negative limit
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(50));
        if (MOT_STATE_WARNING == this->yawMotor->get_state()) {
            vTaskDelay(pdMS_TO_TICKS(100));
            pos_min = this->yawMotor->get_position(); // get negative limit
            break;
        }
    }
    // set motor to midpoint
    this->yawMotor->set_max_speed(old_speed);
    this->yawMotor->set_position((pos_max + pos_min) / 2);
    // wait motor done
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
        float pos = this->yawMotor->get_position();
        if (std::fabs(pos - (pos_max + pos_min) / 2) < 0.1) {
            break;
        }
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
    this->yawMotor->clear_position();
    this->pitchMotor->clear_position();
}


void Gimbal::init()
{
    ESP_LOGI(TAG, "Gimbal initializing");

    this->gps = std::make_shared<GPS>();
    this->gps->init();
    this->imu = std::make_shared<IMUBmi270>();
    uint8_t retry = 10;
    while (retry > 0) {
        if (0 == this->imu->init()) {
            break;
        }
        retry--;
        ESP_LOGW(TAG, "IMU init failed, retrying");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    this->pitchMotor = std::make_shared<Motor>("pitch", 360.0f);
    this->yawMotor = std::make_shared<Motor>("yaw", 3000.0f);
    static EncoderSensor encoderx;
    encoderx.init(BOARD_IO_MOTX_ENC_A, BOARD_IO_MOTX_ENC_B, 4 * 11);
    static IMUMotSensor imusensory;
    imusensory.init(this->imu.get()); // TODO save the params first

    static PWM pwmx, pwmy;
    pwmx.init(BOARD_IO_MOTX_IN1, BOARD_IO_MOTX_IN2, 25000);
    pwmy.init(BOARD_IO_MOTY_IN1, BOARD_IO_MOTY_IN2, 25000);
    this->yawMotor->attach_sensor(&encoderx);
    this->pitchMotor->attach_sensor(&imusensory);
    this->yawMotor->attach_driver(&pwmx);
    this->pitchMotor->attach_driver(&pwmy);

    pid_struct_init(&this->pitchMotor->positionPID, &g_settings.pitch_pos_pid);
    pid_struct_init(&this->pitchMotor->velocityPID, &g_settings.pitch_vel_pid);
    pid_struct_init(&this->yawMotor->positionPID, &g_settings.pos_pid);
    pid_struct_init(&this->yawMotor->velocityPID, &g_settings.vel_pid);

    pid_struct_init(&this->pitchPID, &g_settings.pitch_pid);

    auto gimbal = std::shared_ptr<Gimbal>(this);
    auto logger = std::make_shared<SensorLogger>();
    // this->imu->registerObserver(logger);
    this->imu->registerObserver(gimbal);
    this->gps->registerObserver(gimbal);

    setTarget(0, 0, 0);

    this->pitchMotor->enable(1);
    this->yawMotor->enable(1);
    this->pitchMotor->set_max_speed(100);
    this->yawMotor->set_max_speed(100);

    set_time(2025, 3, 28, 12, 0, 0, 0);
    search_azimuth(&max_azimuth, &min_azimuth, &max_elevation, &min_elevation);

    led_start_state(LED_GREEN, BLINK_FAST);
    state = STATE_HOMING;
    check_home(70);
    led_stop_state(LED_GREEN, BLINK_FAST);
    state = STATE_RUNNING;

    if (g_settings.mode == MODE_MANUAL) {
        setTarget(g_settings.target_pitch, 0, g_settings.target_yaw);
    }

    // create task to update gimbal
    task_sem = xSemaphoreCreateBinary();
    xTaskCreate(Gimbal::update_task, "gimbal_update", 4096, this, 5, NULL);
}

void Gimbal::update(const imu_data_t &data)
{
    static int count = 0;
    if (count++ % 10 == 0) {
        voltage = adc_read_voltage();
        if (state <= STATE_RUNNING) { // only check voltage when not in error state
            if (voltage < g_settings.vol_min) {
                ESP_LOGW(TAG, "Voltage too low: %.2fV", voltage);
                state = STATE_LOW_VOLTAGE;
            } else if (voltage > g_settings.vol_max) {
                ESP_LOGW(TAG, "Voltage too high: %.2fV", voltage);
                state = STATE_HIGH_VOLTAGE;
            }
            if (state > STATE_RUNNING) { // if in error state, stop motors and blink red LED
                ESP_LOGW(TAG, "Gimbal in error state: %s", getStateDescription());
                led_start_state(LED_RED, BLINK_FAST);
                this->pitchMotor->enable(0);
                this->yawMotor->enable(0);
            }
        } else {
            if (voltage >= g_settings.vol_min && voltage <= g_settings.vol_max) { // if voltage is back to normal, reset state
                ESP_LOGW(TAG, "Voltage is back to normal: %.2fV", voltage);
                state = STATE_RUNNING;
                led_stop_state(LED_RED, BLINK_FAST);
                this->pitchMotor->enable(1);
                this->yawMotor->enable(1);
            }
        }
    }

    static uint64_t last_time = 0;
    uint64_t _time = esp_timer_get_time(); // 获取开始时间（微秒级）
    float dt = (float)(_time - last_time) / 1000000.0f;
    last_time = _time;

    // printf("angle: %f %f %f\n", data.angle.x, data.angle.y, data.angle.z);
    // printf("gyro: %f %f %f\n", data.gyro.x, data.gyro.y, data.gyro.z);
    // printf("acc: %f %f %f\n", data.acc.x, data.acc.y, data.acc.z);


    this->pitchMotor->run(dt);
    this->yawMotor->run(dt);
}

void Gimbal::update(const gps_t &data)
{
    static int count = 0;
    if (count++ % 10 == 0) {
        set_time(data.date.year + 2000, data.date.month, data.date.day, data.tim.hour, data.tim.minute, data.tim.second, 1);
    }
}

void Gimbal::getSunPosition(cSunCoordinates *sunCoordinates)
{
    if (nullptr == sunCoordinates) {
        return;
    }
    time_t now;
    struct tm timeinfo;
    time(&now);
    gmtime_r(&now, &timeinfo);
    struct tm localtime;
    localtime_r(&now, &localtime);

    cTime time = {
        .iYear = timeinfo.tm_year + 1900,
        .iMonth = timeinfo.tm_mon + 1,
        .iDay = timeinfo.tm_mday,
        .dHours = (double)(timeinfo.tm_hour),
        .dMinutes = (double)timeinfo.tm_min,
        .dSeconds = (double)timeinfo.tm_sec,
    };
    cLocation location = {
        .dLongitude = gps->getData().longitude,
        .dLatitude = gps->getData().latitude,
    };
    sunpos(time, location, sunCoordinates);
    printf("LocalTime:%d-%d-%d %d:%d:%d UTCTime:%d-%d-%d %d:%d:%d Elevation:%.2f°, Azimuth:%.2f°, Zenith:%.2f°\n", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
           localtime.tm_year + 1900, localtime.tm_mon + 1, localtime.tm_mday, localtime.tm_hour, localtime.tm_min, localtime.tm_sec,
           sunCoordinates->dElevation, sunCoordinates->dAzimuth, sunCoordinates->dZenithAngle
    );
    printf("Longitude:%.2f, Latitude:%.2f\n", location.dLongitude, location.dLatitude);
}

void Gimbal::update_task(void *pvParameters)
{
    auto pgimbal = (Gimbal *)pvParameters;
    while (1) {
        pgimbal->getSunPosition(&pgimbal->sunPosition);
        if (pgimbal->sunPosition.dElevation <= 3) {
            ESP_LOGI(TAG, "Sun is below horizon, setting target to 0");
            pgimbal->setTarget(0, 0, 180);
        } else {
            ESP_LOGI(TAG, "Sun is above horizon setting target");
            switch (g_settings.mode) {
            case MODE_REFLECT: {
                // calculate normal vector of mirror surface
                auto incident = pgimbal->light.angle_to_vector(pgimbal->sunPosition.dAzimuth, pgimbal->sunPosition.dElevation);
                auto reflection_vector = pgimbal->light.angle_to_vector(g_settings.target_yaw + 180, g_settings.target_pitch);
                auto calculated_normal = pgimbal->light.calculate_normal(incident, reflection_vector);
                auto [normal_azimuth, normal_elevation] = pgimbal->light.vector_to_angle(calculated_normal);
                ESP_LOGI(TAG, "Normal Vector Azimuth: %f, Elevation: %f", normal_azimuth, normal_elevation);
                pgimbal->setTarget(90 - normal_elevation, 0, normal_azimuth);
            } break;

            case MODE_TOWARD: {
                pgimbal->setTarget(pgimbal->sunPosition.dZenithAngle, 0, pgimbal->sunPosition.dAzimuth);
            } break;

            case MODE_MANUAL:
                pgimbal->setTarget(g_settings.target_pitch, 0, g_settings.target_yaw+180);
                break;

            default:
                break;
            }
        }
        xSemaphoreTake(pgimbal->task_sem, pdMS_TO_TICKS(10000));
    }
}

void Gimbal::triger_task_immediate()
{
    xSemaphoreGive(task_sem);
}

void Gimbal::setTarget(float pitch, float roll, float yaw)
{
    // unit: degree 0 - 360
    const float Aziimuth_mid = 180;
    this->pitchTarget = pitch;
    this->yawTarget = yaw;
    this->yawMotor->set_position((yawTarget + g_settings.yaw_offset) - Aziimuth_mid);
    this->pitchMotor->set_position(pitchTarget);
}

void Gimbal::search_azimuth(float *max_azimuth, float *min_azimuth, float *max_elevation, float *min_elevation)
{
    *max_azimuth = 0;
    *min_azimuth = 360;
    *max_elevation = -360;
    *min_elevation = 360;
    // search maximum and minimum azimuth in a day
    time_t now;
    struct tm local_time;
    time(&now);
    localtime_r(&now, &local_time);
    printf("Searching for Local Time: %d-%02d-%02d %02d:%02d:%02d\n", local_time.tm_year + 1900, local_time.tm_mon + 1, local_time.tm_mday, local_time.tm_hour, local_time.tm_min, local_time.tm_sec);

    local_time.tm_hour = 6;
    local_time.tm_min = 0;
    local_time.tm_sec = 0;
    time_t start_time = mktime(&local_time);

    cLocation location = {
        .dLongitude = gps->getData().longitude,
        .dLatitude = gps->getData().latitude,
    };

    // 模拟从6点到18点每隔30分钟计算一次
    for (int i = 0; i < 25; i++) { // 从6点到18点有25个
        // 转换为UTC时间
        struct tm utcTime;
        gmtime_r(&start_time, &utcTime);

        // 转换为cTime结构体
        cTime utcTimeStruct = {
            .iYear = utcTime.tm_year + 1900,
            .iMonth = utcTime.tm_mon + 1,
            .iDay = utcTime.tm_mday,
            .dHours = (double)utcTime.tm_hour,
            .dMinutes = (double)utcTime.tm_min,
            .dSeconds = (double)utcTime.tm_sec
        };

        cSunCoordinates sunCoordinates;
        sunpos(utcTimeStruct, location, &sunCoordinates);

        // 更新Azimuth的最大值和最小值
        if (sunCoordinates.dAzimuth > *max_azimuth) {
            *max_azimuth = sunCoordinates.dAzimuth;
        }
        if (sunCoordinates.dAzimuth < *min_azimuth) {
            *min_azimuth = sunCoordinates.dAzimuth;
        }

        // 更新Elevation的最大值和最小值
        if (sunCoordinates.dElevation > *max_elevation) {
            *max_elevation = sunCoordinates.dElevation;
        }
        if (sunCoordinates.dElevation < *min_elevation) {
            *min_elevation = sunCoordinates.dElevation;
        }

        // 打印结果
        printf("UTC Time: %d-%02d-%02d %02d:%02d:%02d  Elevation: %.2f°, Azimuth: %.2f°, Zenith: %.2f°\n",
               utcTimeStruct.iYear, utcTimeStruct.iMonth, utcTimeStruct.iDay, (int)utcTimeStruct.dHours, (int)utcTimeStruct.dMinutes, (int)utcTimeStruct.dSeconds,
               sunCoordinates.dElevation, sunCoordinates.dAzimuth, sunCoordinates.dZenithAngle);

        // 更新时间为下一个30分钟
        start_time += 30 * 60;
    }

    printf("Azimuth maximum:%.2f°, minimum:%.2f°, midpoint:%.2f\n", *max_azimuth, *min_azimuth, (*max_azimuth + *min_azimuth) / 2);
    printf("Elevation maximum:%.2f°, minimum:%.2f°, midpoint:%.2f\n", *max_elevation, *min_elevation, (*max_elevation + *min_elevation) / 2);
}
