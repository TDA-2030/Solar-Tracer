/*

  */
#ifndef __SETTING__H_
#define __SETTING__H_

#include <stdint.h>
#include <string>
#include <limits.h>
#include <unordered_map>
#include "pid.h"
#include "esp_err.h"

#define SETTINGS_NAMESPACE "settings"
#define SETTINGS_KEY "main"

enum {
  MODE_MANUAL = 0,
  MODE_TOWARD = 1,
  MODE_REFLECT = 2,
};

template <typename T>
class Parameter
{
protected:
    std::string name; // 参数名称
    T &value; // 参数值
    const T &defaultValue; // 默认值
    size_t length; // 参数长度

public:
    Parameter(std::string _name, T &_value, const T &_defaultValue, size_t _length): 
    name(_name), value(_value), defaultValue(_defaultValue), length(_length) {}

private:
    Parameter(const Parameter &other) = delete;
    // Parameter &operator=(const Parameter &other) = delete;
    Parameter(Parameter &&other) = delete;
};

class Setting {
public:
    Setting();
    esp_err_t load();
    esp_err_t save();

    // // 注册参数及其默认值
    // template <typename T>
    // void registerParameter(const std::string &name, const T &Value, const T &defaultValue)
    // {
    //     // 检查参数是否已经注册
    //     if (parameters.find(name) != parameters.end())
    //     {
    //         throw std::runtime_error("Parameter already registered: " + name);
    //     }

    //     // 注册参数
    //     parameters[name] = Parameter<T>(name, &Value, &defaultValue, sizeof(T));
    // }


    // // 更新参数值
    // template <typename T>
    // void setParameter(const std::string &name, const T &newValue)
    // {
    //     auto it = parameters.find(name);
    //     if (it == parameters.end())
    //     {
    //         throw std::runtime_error("Parameter not found: " + name);
    //     }
    //     if (it->second.length != sizeof(T)) {
    //         throw std::runtime_error("Parameter length mismatch: " + name);
    //     }
    //     it->second.value = newValue;
    // }

    // // 查询参数值
    // template <typename T>
    // const T& getParameter(const std::string &name) const
    // {
    //     auto it = parameters.find(name);
    //     if (it == parameters.end())
    //     {
    //         throw std::runtime_error("Parameter not found: " + name);
    //     }
    //     if (it->second.length != sizeof(T)) {
    //         throw std::runtime_error("Parameter length mismatch: " + name);
    //     }

    //     return it->second.value;
    // }

    // Public member variables
    uint8_t mode;
    struct pid_param pos_pid;
    struct pid_param vel_pid;
    struct pid_param pitch_pos_pid;
    struct pid_param pitch_vel_pid;

    struct pid_param pitch_pid;
    float vol_max;
    float vol_min;
    float target_pitch;
    float target_yaw;

    float yaw_offset; // degrees
    float magnetic_declination_degrees;

private:
    // std::unordered_map<std::string, Parameter> parameters; // 存储所有参数
    uint32_t checksum;  // Must be the first member for checksum calculation
    void restortDefault();
    void print();
    bool validateChecksum();
    void updateChecksum();
    uint32_t calculateChecksum();
    bool validateRanges();
};

extern Setting g_settings;

#endif
