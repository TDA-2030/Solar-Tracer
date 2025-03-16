/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#ifndef __IMU_BMI270_H_
#define __IMU_BMI270_H_

#include "imu_base.h"
#include "bmi270.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif


class IMUBmi270 : public IMUBase {
public:
    IMUBmi270();
    ~IMUBmi270();

    void readData();
private:
    bmi270_handle_t bmi_handle = nullptr;

    TaskHandle_t imuTaskHandle;

    // IMUBmi270(const IMUBmi270 &) = delete;
    // IMUBmi270 &operator=(const IMUBmi270 &) = delete;
};



#endif
