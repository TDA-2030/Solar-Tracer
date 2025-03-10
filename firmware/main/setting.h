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

    struct pid_param pitch_pid;
    float vol_max;
    float vol_min;
    float target_pitch;
    float target_yaw;

    float yaw_offset; // degrees

private:
    uint32_t checksum;  // Must be the first member for checksum calculation
    bool validateChecksum();
    void updateChecksum();
    uint32_t calculateChecksum();
    bool validateRanges();
};

extern Setting g_settings;

#endif
