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
#include "freertos/semphr.h"
#include "observer.hpp"
#include "imu_bmi270.h"
#include "motor.h"
#include "pid.h"
#include "gps.h"
#include "light_reflection.hpp"
#include "sun_pos.h"

#define SYS_STATE_LIST \
X(STATE_INIT, "Initial")   \
X(STATE_HOMING, "Homing")   \
X(STATE_RUNNING, "Running")     \
X(STATE_LOW_VOLTAGE, "Error Low voltage") \
X(STATE_HIGH_VOLTAGE, "Error High voltage") \

enum SysState {
#define X(name, desc) name,
    SYS_STATE_LIST
#undef X
    SYS_STATE_COUNT
};

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
    float getPitchTarget(){ return pitchTarget;}
    float getYawTarget(){return yawTarget;}
    void check_home(float homing_speed);

    void update(const gps_t &data) override;
    void update(const imu_data_t &data) override;
    void search_azimuth(float *max_azimuth, float *min_azimuth, float *max_elevation, float *min_elevation);
    void getSunPosition(cSunCoordinates *sunCoordinates);
    void triger_task_immediate();
    std::shared_ptr<IMUBmi270> imu;
    std::shared_ptr<GPS> gps;
    float voltage = 0.0f; // 电压
    struct pid pitchPID;
    std::shared_ptr<Motor> pitchMotor;
    std::shared_ptr<Motor> yawMotor;
    cSunCoordinates sunPosition;
    const char *getStateDescription() const
    {
        return SysStateDescriptions[state];
    }

private:
    static const char* SysStateDescriptions[];
    SysState state = STATE_INIT;
    static void update_task(void *pvParameters);
    SemaphoreHandle_t task_sem;
    LightReflection light;
    float pitchTarget;
    float yawTarget;
    float max_azimuth, min_azimuth, max_elevation, min_elevation;
};

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
