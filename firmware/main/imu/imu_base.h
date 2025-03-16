#pragma once


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
        axis_t acc;   // 加速度
        axis_t gyro;  // 角速度
        axis_t angle; // 角度
    };
    struct {
        axis_t data[3];
    };
} imu_data_t;

#ifdef __cplusplus


#include "observer.hpp"

class IMUBase : public Subject<imu_data_t> {
public:
    virtual ~IMUBase() = default;

    // 数据访问
    const imu_data_t& getData() const { return imu_data; }

protected:
    imu_data_t imu_data;
};


#endif

