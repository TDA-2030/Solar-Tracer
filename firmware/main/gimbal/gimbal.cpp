
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gimbal.h"
#include "esp_log.h"

Gimbal::Gimbal(): imu(nullptr), rollMotor(nullptr), pitchMotor(nullptr), yawMotor(nullptr),
rollTarget(0), pitchTarget(0), yawTarget(0)

{
    ESP_LOGI("GIMBAL", "Gimbal created");
}

Gimbal::~Gimbal()
{

}


void Gimbal::init()
{
    ESP_LOGI("GIMBAL", "Gimbal initializing");
    static Motor pitchMotor(0);
    static Motor rollMotor(1);

    this->imu = std::make_shared<IMU>(); 
    this->pitchMotor = &pitchMotor;
    this->rollMotor = &rollMotor;
    this->yawMotor = nullptr;

    pid_struct_init(&positionPID[0], 0.1, 0.01, 0.01, 0, 0);
    pid_struct_init(&positionPID[1], 0.1, 0.01, 0.01, 0, 0);
    pid_struct_init(&velocityPID[0], 0.1, 0.01, 0.01, 0, 0);
    pid_struct_init(&velocityPID[1], 0.1, 0.01, 0.01, 0, 0);

    auto gimbal = std::shared_ptr<Gimbal>(this);
    auto logger = std::make_shared<SensorLogger>();
    this->imu->registerObserver(gimbal);
    this->imu->registerObserver(logger);
}


void Gimbal::update(const imu_data_t& data)
{
    axis_t speed_target;
    axis_t output;
    speed_target.x = pid_calculate(&positionPID[0], data.angle.x, rollTarget);
    speed_target.y = pid_calculate(&positionPID[1], data.angle.y, pitchTarget);

    output.x = pid_calculate(&velocityPID[0], data.gyro.x, speed_target.x);
    output.y = pid_calculate(&velocityPID[1], data.gyro.y, speed_target.y);

    pitchMotor->set_pwm(output.x);
    rollMotor->set_pwm(output.y);

}

void Gimbal::setTarget(float pitch, float roll, float yaw)
{
    this->pitchTarget = pitch;
    this->rollTarget = roll;
    this->yawTarget = yaw;
}


