/*

  */
#ifndef __SETTING__H_
#define __SETTING__H_

#include <stdint.h>
#include "pid.h"
#include "esp_err.h"

#define SETTINGS_NAMESPACE "settings"
#define SETTINGS_KEY "main"

enum {
  MODE_MANUAL = 0,
  MODE_AUTO = 1
};

class Setting {
public:
    Setting();
    esp_err_t load();
    esp_err_t save();

    // Public member variables
    uint8_t mode;
    struct pid_param pos_pid;
    struct pid_param vel_pid;
    float vol_max;
    float vol_min;
    float target_pitch;
    float target_yaw;
};

extern Setting g_settings;

#endif
