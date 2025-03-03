/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "imu.h"
#include "motor.h"
#include "pid.h"
#include "observer.hpp"
#include <iostream>

class SensorLogger : public Observer<imu_data_t> {
public:
    void update(const imu_data_t &data) override
    {
        printf("angle:%.3f %.3f %.3f\r\n", data.angle.data[0], data.angle.data[1], data.angle.data[2]);
        printf("acc:%.3f %.3f %.3f\r\n", data.acc.data[0], data.acc.data[1], data.acc.data[2]);
        printf("gyro:%.3f %.3f %.3f\r\n", data.gyro.data[0], data.gyro.data[1], data.gyro.data[2]);
    }
};


class Gimbal : public Observer<imu_data_t> {
public:
    Gimbal();
    ~Gimbal();
    void init();
    void setTarget(float pitch, float roll, float yaw);

    void update(const imu_data_t &data) override;
private:

    struct pid positionPID[3], velocityPID[3];

    std::shared_ptr<IMU> imu;
    Motor *rollMotor;
    Motor *pitchMotor;
    Motor *yawMotor;

    float rollTarget;
    float pitchTarget;
    float yawTarget;
};

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
