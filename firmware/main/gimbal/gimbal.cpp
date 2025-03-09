
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "gimbal.h"
#include "setting.h"

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

    pid_struct_init(&positionPID[0], &g_settings.pos_pid);
    pid_struct_init(&positionPID[1], &g_settings.pos_pid);
    pid_struct_init(&velocityPID[0], &g_settings.vel_pid);
    pid_struct_init(&velocityPID[1], &g_settings.vel_pid);

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

    axis_t angle;
    axis_t gyro;
    angle.y = -data.angle.y;
    gyro.y = -data.gyro.y;
    angle.z = data.angle.z;
    gyro.z = data.gyro.z;

    axis_t speed_target;
    axis_t output;
    speed_target.y = pid_calculate(&positionPID[0], angle.y, pitchTarget, dt);
    speed_target.z = pid_calculate(&positionPID[1], angle.z, yawTarget, dt);

    output.y = pid_calculate(&velocityPID[0], gyro.y, speed_target.y, dt);
    output.z = pid_calculate(&velocityPID[1], gyro.z, speed_target.z, dt);

    printf("out: %f %f | %f %f\n", angle.y, output.y, angle.z, output.z);
    pitchMotor->set_pwm(output.y);
    yawMotor->set_pwm(output.z);

}

void Gimbal::setTarget(float pitch, float roll, float yaw)
{
    this->pitchTarget = pitch;
    this->rollTarget = roll;
    this->yawTarget = yaw;
}


