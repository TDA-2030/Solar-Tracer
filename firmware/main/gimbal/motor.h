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

typedef enum {
    MOT_STATE_IDLE = 0,
    MOT_STATE_RUNNING,
    MOT_STATE_WARNING,
}mot_state_t;

class Motor {
public:
    Motor(uint8_t _mot_id, float _cpr);

    // 禁止拷贝构造和赋值操作
    Motor(const Motor &) = delete;
    Motor &operator=(const Motor &) = delete;

    void run(float dt); // 运行电机，周期调用
    void enable(bool is_enable);
    void get_postion(float *revolutions); // 获取电机当前位置(圈数)
    void get_velocity(float *velocity);  // 获取电机当前速度(圈/秒)
    void set_postion(float position);

    struct pid positionPID;
    struct pid velocityPID;
    mot_state_t state; // 电机状态
private:
    uint8_t mot_id;
    pcnt_unit_handle_t pcnt_unit;
    float cpr; // 电机编码器分辨率
    float last_revolutions = 0;
    float current_speed;
    float target_speed;
    float target_position;

    void set_pwm(int32_t percent);

    static bool is_init;
};

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif
