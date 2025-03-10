
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "gimbal.h"
#include "setting.h"

static const char *TAG = "gimbal";

Gimbal::Gimbal(): imu(nullptr), pitchMotor(nullptr), yawMotor(nullptr),
    pitchTarget(0), yawTarget(0)

{
    ESP_LOGI(TAG, "Gimbal created");
}

Gimbal::~Gimbal()
{

}


void Gimbal::init()
{
    ESP_LOGI(TAG, "Gimbal initializing");

    this->imu = std::make_shared<IMU>();
    this->pitchMotor = std::make_shared<Motor>(0, 11*200);
    this->yawMotor = std::make_shared<Motor>(1, 11*200);

    pid_struct_init(&this->pitchMotor->positionPID, &g_settings.pos_pid);
    pid_struct_init(&this->pitchMotor->velocityPID, &g_settings.vel_pid);
    pid_struct_init(&this->yawMotor->positionPID, &g_settings.pos_pid);
    pid_struct_init(&this->yawMotor->velocityPID, &g_settings.vel_pid);

    pid_struct_init(&this->pitchPID, &g_settings.pitch_pid);

    auto gimbal = std::shared_ptr<Gimbal>(this);
    auto logger = std::make_shared<SensorLogger>();
    // this->imu->registerObserver(logger);
    this->imu->registerObserver(gimbal);

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

    // 只有pitch轴是由imu角度反馈控制的
    float output = pid_calculate(&pitchPID, data.angle.y, pitchTarget, dt);
    this->pitchMotor->set_postion(output);
    this->yawMotor->set_postion(yawTarget + g_settings.yaw_offset);
    this->pitchMotor->run(dt);
    this->yawMotor->run(dt);
}

void Gimbal::setTarget(float pitch, float roll, float yaw)
{
    this->pitchTarget = pitch;
    this->yawTarget = yaw;
}


