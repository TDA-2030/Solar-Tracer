/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once

#include <iostream>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "observer.hpp"
#include "imu_bmi270.h"
#include "qmc5883p.h"
#include "motor.h"
#include "pid.h"
#include "gps.h"

class SensorLogger : public Observer<imu_data_t> {
public:
    void update(const imu_data_t &data) override
    {
        printf( "angle:%8.3f %8.3f %8.3f\r\n", data.angle.data[0], data.angle.data[1], data.angle.data[2]);
        printf( "acc:  %8.3f %8.3f %8.3f\r\n", data.acc.data[0], data.acc.data[1], data.acc.data[2]);
        printf( "gyro: %8.3f %8.3f %8.3f\r\n", data.gyro.data[0], data.gyro.data[1], data.gyro.data[2]);
        printf("\r\n");
    }
};


class Gimbal : public Observer<imu_data_t>, public Observer<gps_t> {
public:
    Gimbal();
    ~Gimbal();
    void init();
    void setTarget(float pitch, float roll, float yaw);
    void check_home(float homing_speed);

    void update(const gps_t &data) override;
    void update(const imu_data_t &data) override;
    std::shared_ptr<IMUBmi270> imu;
    std::shared_ptr<AP_Compass_QMC5883P> compass;
    std::shared_ptr<GPS> gps;
    struct pid pitchPID;
    std::shared_ptr<Motor> pitchMotor;
    std::shared_ptr<Motor> yawMotor;
    gps_t gpsData;

private:
    float gearRatio;
    float pitchTarget;
    float yawTarget;
};

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
