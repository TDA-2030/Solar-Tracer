
#include <memory>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "helper.h"
#include "gimbal.h"
#include "setting.h"
#include "sun_pos.h"
#include "led.h"

static const char *TAG = "gimbal";

#define LPF(beta, prev, input) ((beta) * (input) + (1 - (beta)) * (prev))

Gimbal::Gimbal(): imu(nullptr), gps(nullptr), pitchMotor(nullptr), yawMotor(nullptr),
    pitchTarget(0), yawTarget(0)

{
    gearRatio = 3000.0f;
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
    this->yawMotor->set_position(2 * gearRatio); // set to 2*gearRatio to make sure it is reach positive limit
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(50));
        if (MOT_STATE_WARNING == this->yawMotor->get_state()) {
            vTaskDelay(pdMS_TO_TICKS(100));
            this->yawMotor->get_position(&pos_max); // get positive limit
            break;
        }
    }
    this->yawMotor->set_position(-2 * gearRatio); // set to -2*gearRatio to make sure it is reach negative limit
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(50));
        if (MOT_STATE_WARNING == this->yawMotor->get_state()) {
            vTaskDelay(pdMS_TO_TICKS(100));
            this->yawMotor->get_position(&pos_min); // get negative limit
            break;
        }
    }
    // set motor to midpoint
    this->yawMotor->set_max_speed(old_speed);
    this->yawMotor->set_position((pos_max + pos_min) / 2);
    // wait motor done
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
        float pos;
        this->yawMotor->get_position(&pos);
        if (fabs(pos - (pos_max + pos_min) / 2) < 0.1) {
            break;
        }
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
    this->yawMotor->clear_position();
}


void Gimbal::init()
{
    ESP_LOGI(TAG, "Gimbal initializing");

    this->gps = std::make_shared<GPS>();

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
    this->compass = std::make_shared<AP_Compass_QMC5883P>();
    this->pitchMotor = std::make_shared<Motor>(1, 4 * 11);
    this->yawMotor = std::make_shared<Motor>(0, 4 * 11);

    pid_struct_init(&this->pitchMotor->positionPID, &g_settings.pos_pid);
    pid_struct_init(&this->pitchMotor->velocityPID, &g_settings.vel_pid);
    pid_struct_init(&this->yawMotor->positionPID, &g_settings.pos_pid);
    pid_struct_init(&this->yawMotor->velocityPID, &g_settings.vel_pid);

    pid_struct_init(&this->pitchPID, &g_settings.pitch_pid);

    auto gimbal = std::shared_ptr<Gimbal>(this);
    auto logger = std::make_shared<SensorLogger>();
    // this->imu->registerObserver(logger);
    this->imu->registerObserver(gimbal);
    this->gps->registerObserver(gimbal);

    this->pitchMotor->enable(1);
    this->yawMotor->enable(1);
    this->pitchMotor->set_max_speed(100);
    this->yawMotor->set_max_speed(100);

    gpsData.longitude = 112.933333;
    gpsData.latitude = 28.183333;
    set_time(2025, 3, 28, 10, 0, 0);
    float max_azimuth, min_azimuth;
    search_azimuth(&max_azimuth, &min_azimuth);

    led_start_state(LED_GREEN, BLINK_FAST);
    check_home(70);
    led_stop_state(LED_GREEN, BLINK_FAST);

    if (g_settings.mode == MODE_MANUAL) {
        setTarget(g_settings.target_pitch, 0, g_settings.target_yaw);
    }

    // create task to update gimbal
    xTaskCreate(Gimbal::update_task, "gimbal_update", 4096, this, 5, NULL);
}

void Gimbal::update(const imu_data_t &data)
{
    static int count = 0;
    if (count++ % 100 == 0) {
        static uint64_t _lt = 0;
        uint64_t start_time = esp_timer_get_time(); // 获取开始时间（微秒级）
        ESP_LOGI(TAG, "Gimbal updating %d", (int)((start_time - _lt) / 1000));
        _lt = start_time;
    }

    static uint64_t last_time = 0;
    uint64_t _time = esp_timer_get_time(); // 获取开始时间（微秒级）
    float dt = (float)(_time - last_time) / 1000000.0f;
    last_time = _time;

    // compass->read();

    // printf("angle: %f %f %f\n", data.angle.x, data.angle.y, data.angle.z);
    // printf("gyro: %f %f %f\n", data.gyro.x, data.gyro.y, data.gyro.z);
    // printf("acc: %f %f %f\n", data.acc.x, data.acc.y, data.acc.z);

    // 只有pitch轴是由imu角度反馈控制的
    static float last_spd_out = 0;
    float output = pid_calculate(&pitchPID, data.angle.y, pitchTarget, dt);
    last_spd_out = LPF(0.75, last_spd_out, output);
    this->pitchMotor->set_position(last_spd_out);
    this->pitchMotor->run(dt);
    this->yawMotor->run(dt);
}

void Gimbal::update(const gps_t &data)
{
    gpsData = data;
    static int count = 0;
    if (count++ % 10 == 0) {
        ESP_LOGI(TAG, "Gimbal updating gps, UTC Time:%d/%d/%d %d:%d:%d "
                 "latitude = %.05f° "
                 "longitude = %.05f° "
                 "altitude = %.02fm "
                 "speed = %fm/s",
                 gpsData.date.year + 2000, gpsData.date.month, gpsData.date.day,
                 gpsData.tim.hour, gpsData.tim.minute, gpsData.tim.second,
                 gpsData.latitude, gpsData.longitude, gpsData.altitude, gpsData.speed);
        set_time(data.date.year + 2000, data.date.month, data.date.day, data.tim.hour, data.tim.minute, data.tim.second);
    }
}

void Gimbal::update_task(void *pvParameters)
{
    auto pgimbal = (Gimbal *)pvParameters;
    while (1) {
        time_t now;
        struct tm timeinfo;
        time(&now);
        gmtime_r(&now, &timeinfo);

        cTime time = {
            .iYear = timeinfo.tm_year + 1900,
            .iMonth = timeinfo.tm_mon + 1,
            .iDay = timeinfo.tm_mday,
            .dHours = (double)(timeinfo.tm_hour),
            .dMinutes = (double)timeinfo.tm_min,
            .dSeconds = (double)timeinfo.tm_sec,
        };
        cLocation location = {
            .dLongitude = pgimbal->gpsData.longitude,
            .dLatitude = pgimbal->gpsData.latitude,
        };
        cSunCoordinates sunCoordinates;
        sunpos(time, location, &sunCoordinates);
        ESP_LOGI(TAG, "UTC Time:%d-%d-%d %d:%d:%d  Elevation:%f°, Azimuth:%f°, Zenith:%f°", time.iYear, time.iMonth, time.iDay, (int)time.dHours, (int)time.dMinutes, (int)time.dSeconds,
                 sunCoordinates.dElevation, sunCoordinates.dAzimuth, sunCoordinates.dZenithAngle);
        if (g_settings.mode == MODE_AUTO) {
            if (sunCoordinates.dElevation > 0) {
                ESP_LOGI(TAG, "Sun is above horizon setting target");
                pgimbal->setTarget(sunCoordinates.dZenithAngle, 0, 0);
            } else {
                ESP_LOGI(TAG, "Sun is below horizon");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void Gimbal::setTarget(float pitch, float roll, float yaw)
{
    // unit: degree 0 - 360
    this->pitchTarget = pitch;
    this->yawTarget = yaw;
    this->yawMotor->set_position((yawTarget + g_settings.yaw_offset) * gearRatio / 360);
}

void Gimbal::search_azimuth(float *max_azimuth, float *min_azimuth)
{
    *max_azimuth = 0;
    *min_azimuth = 360;
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

    // 假设的 GPS 数据
    cLocation location = {
        .dLongitude = gpsData.longitude,
        .dLatitude = gpsData.latitude,
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

        // 打印结果
        printf("UTC Time: %d-%02d-%02d %02d:%02d:%02d  Elevation: %.2f°, Azimuth: %.2f°, Zenith: %.2f°\n",
               utcTimeStruct.iYear, utcTimeStruct.iMonth, utcTimeStruct.iDay, (int)utcTimeStruct.dHours, (int)utcTimeStruct.dMinutes, (int)utcTimeStruct.dSeconds,
               sunCoordinates.dElevation, sunCoordinates.dAzimuth, sunCoordinates.dZenithAngle);

        // 更新时间为下一个30分钟
        start_time += 30 * 60;
    }

    printf("Azimuth maximum: %.2f°, minimum: %.2f°\n", *max_azimuth, *min_azimuth);
}
