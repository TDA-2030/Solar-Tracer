
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "gimbal.h"

static const char *TAG = "gimbal";

Gimbal::Gimbal(): imu(nullptr), rollMotor(nullptr), pitchMotor(nullptr), yawMotor(nullptr),
    rollTarget(0), pitchTarget(0), yawTarget(0)

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
    this->pitchMotor = std::make_shared<Motor>(0);
    this->rollMotor = nullptr;
    this->yawMotor = std::make_shared<Motor>(1);

    pid_struct_init(&positionPID[0], 100, 300, 0.01, 0, 0);
    pid_struct_init(&positionPID[1], 100, 300, 0.01, 0, 0);
    pid_struct_init(&velocityPID[0], 1000, 300, 0.01, 0, 0);
    pid_struct_init(&velocityPID[1], 1000, 300, 0.01, 0, 0);

    auto gimbal = std::shared_ptr<Gimbal>(this);
    auto logger = std::make_shared<SensorLogger>();
    // this->imu->registerObserver(logger);
    this->imu->registerObserver(gimbal);
}

void Gimbal::update(const imu_data_t &data)
{
    static int count = 0;
    if (count++ % 100 == 0) {
        static uint64_t last_time = 0;
        uint64_t start_time = esp_timer_get_time(); // 获取开始时间（微秒级）
        ESP_LOGI(TAG, "Gimbal updating %d", (int)((start_time - last_time) / 1000));
        last_time = start_time;
    }

    axis_t speed_target;
    axis_t output;
    speed_target.y = pid_calculate(&positionPID[0], data.angle.y, pitchTarget);
    speed_target.z = pid_calculate(&positionPID[1], data.angle.z, yawTarget);

    output.y = pid_calculate(&velocityPID[0], data.gyro.y, speed_target.y);
    output.z = pid_calculate(&velocityPID[1], data.gyro.z, speed_target.z);

    pitchMotor->set_pwm(output.y);
    yawMotor->set_pwm(output.z);

}

void Gimbal::setTarget(float pitch, float roll, float yaw)
{
    this->pitchTarget = pitch;
    this->rollTarget = roll;
    this->yawTarget = yaw;
}


