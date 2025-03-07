#pragma once

#include <algorithm>
#include "observer.hpp"


// 绕IMU的Z轴旋转：航向角yaw， 转动 y 角度
// 绕IMU的Y轴旋转：俯仰角pitch，转动 p 角度
// 绕IMU的X轴旋转：横滚角roll， 转动 r 角度
typedef union {
    struct {
        float x;
        float y;
        float z;
    };
    float data[3];
} axis_t;

typedef union {
    struct {
        axis_t acc; // 加速度
        axis_t gyro; // 角速度
        axis_t angle; // 角度
    };
    struct {
        axis_t data[3];
    };
} imu_data_t;

class IMU : public Subject<imu_data_t> {
public:
    IMU();
    ~IMU();

    int startAccCali();
    int startMagCali();
    int stopMagCali();
    int setBandwidth(int bandwidth);
    int setUartBaud(int baud);
    int setOutputRate(int rate);
    int setContent(int content);

    void registerObserver(std::shared_ptr<Observer<imu_data_t>> observer) override
    {
        observers.push_back(observer);
    }

    void removeObserver(std::shared_ptr<Observer<imu_data_t>> observer) override
    {
        auto it = std::find(observers.begin(), observers.end(), observer);
        if (it != observers.end()) {
            observers.erase(it);
        }
    }

    void notifyObservers(const imu_data_t &data) override
    {
        for (const auto &observer : observers) {
            observer->update(data);
        }
    }

    imu_data_t imu_data;

private:
    int uart_num;
    TaskHandle_t imuTaskHandle;
};

