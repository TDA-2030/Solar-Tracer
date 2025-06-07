/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once
#include <stdint.h>
#include "driver/pulse_cnt.h"
#include "pid.h"
#include "imu_base.h"
#include "setting.h"

#define MOT_STATE_LIST \
X(MOT_STATE_IDLE, "Idle") \
X(MOT_STATE_RUNNING, "Running") \
X(MOT_STATE_WARNING, "Warning") \

typedef enum {
#define X(name, desc) name,
    MOT_STATE_LIST
#undef X
    MOT_STATE_COUNT
} mot_state_t;


class PWM {
public:
    PWM() = default;
    ~PWM() = default;

    // 初始化PWM
    bool init(int pin1, int pin2, int freq_hz);

    // 设置PWM占空比
    void set_pwm(int32_t percent);
private:
    int mot_id;
};


class MotorSensor {
public:
    virtual ~MotorSensor() = default;

    // 初始化传感器
    // virtual bool init() = 0;

    // 获取位置（圈数）
    virtual float get_position() = 0;

    // 获取速度（圈/秒）
    virtual float get_velocity() = 0;

    // 清除位置计数
    virtual void clear_position() = 0;

    // 更新速度
    virtual void update_velocity(float dt) = 0;

    // 设置每圈的计数分辨率
    void set_counts_per_rev(float cpr)
    {
        counts_per_rev = cpr;
    }

    // 获取每圈的计数分辨率
    float get_counts_per_rev() const
    {
        return counts_per_rev;
    }

protected:
    float counts_per_rev = 0.0f;  // 每圈计数值
};

class EncoderSensor : public MotorSensor {
public:
    EncoderSensor() {};
    virtual ~EncoderSensor() {};

    bool init(int gpio_enca, int gpio_encb, float cpr);

    float get_position() override;
    float get_velocity() override;
    void clear_position() override;
    void update_velocity(float dt) override;

private:
    pcnt_unit_handle_t pcnt_unit;
    float last_revolutions = 0.0f;
    float cur_velocity = 0.0f;
    float current_revolutions = 0.0f;
};

class IMUMotSensor : public MotorSensor {
public:
    IMUMotSensor() {};
    virtual ~IMUMotSensor() {};

    bool init(IMUBase *_imu);

    float get_position() override;
    float get_velocity() override;
    void clear_position() override;
    void update_velocity(float dt) override;

private:
    IMUBase *imu;
    float cur_velocity = 0.0f;
    float current_revolutions = 0.0f;
};


class Motor {
public:
    Motor(const char *name);
    Motor(const char *name, float gearRatio) : Motor(name)
    {
        this->gearRatio = gearRatio;
    }

    // 禁止拷贝构造和赋值操作
    Motor(const Motor &) = delete;
    Motor &operator=(const Motor &) = delete;

    void attach_sensor(MotorSensor *sensor);
    void attach_driver(PWM *pwm);

    void run(float dt); // 运行电机，周期调用
    void enable(bool is_enable);
    void set_position(float position)
    {
        this->target_position = position * gearRatio / 360.0f;
    }

    float get_position() 
    {
        return this->sensor->get_position() * 360.0f / gearRatio;
    }
    void set_max_speed(float max_speed)
    {
        this->max_speed = max_speed;
    }
    float get_max_speed()
    {
        return this->max_speed;
    }
    mot_state_t get_state()
    {
        return this->state;
    }
    const char *get_state_description() const
    {
        return motStateDescriptions[state];
    }

    float get_velocity()
    {
        return this->sensor->get_velocity();
    }
    void clear_position()
    {
        this->sensor->clear_position();
    }

    struct pid positionPID;
    struct pid velocityPID;
private:
    static const char *motStateDescriptions[];
    const char *name;
    MotorSensor *sensor;
    PWM *pwm;
    float target_speed;
    float target_position;
    uint32_t stall_cnt = 0;
    mot_state_t state; // 电机状态
    float max_speed; // 最大速度
    float gearRatio = 1.0f; // 齿轮比, default 1

};

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif
