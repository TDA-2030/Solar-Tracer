
#include <memory>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "gimbal.h"
#include "setting.h"
#include "sun_pos.h"

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

    check_home(70);

    if (g_settings.mode == MODE_MANUAL) {
        setTarget(g_settings.target_pitch, 0, g_settings.target_yaw);
    }
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
    last_spd_out = LPF(0.7, last_spd_out, output);
    this->pitchMotor->set_position(last_spd_out);
    this->pitchMotor->run(dt);
    this->yawMotor->run(dt);
}

void Gimbal::update(const gps_t &data)
{
    gpsData = data;
    cTime time = {
        .iYear = 2025,//data.date.year + 2000,
        .iMonth = 3,//data.date.month,
        .iDay = 27,//data.date.day,
        .dHours = (double)(data.tim.hour + 8),
        .dMinutes = (double)data.tim.minute,
        .dSeconds = 0,//(double)data.tim.second,
    };
    cLocation location = {
        .dLongitude = 112.933333,//data.longitude,
        .dLatitude = 28.183333,//data.latitude,
    };
    cSunCoordinates sunCoordinates;
    sunpos(time, location, &sunCoordinates);
    ESP_LOGI(TAG, "Zenith Angle:%f, Azimuth:%f", sunCoordinates.dZenithAngle, sunCoordinates.dAzimuth);
}

void Gimbal::setTarget(float pitch, float roll, float yaw)
{
    // unit: degree 0 - 360
    this->pitchTarget = pitch;
    this->yawTarget = yaw;
    this->yawMotor->set_position((yawTarget + g_settings.yaw_offset) * gearRatio / 360);
}


