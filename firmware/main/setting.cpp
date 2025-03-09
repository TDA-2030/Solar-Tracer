#include <string.h>
#include "esp_log.h"
#include "setting.h"
#include "helper.h"

static const char* TAG = "setting";

Setting g_settings;

Setting::Setting() {
    
}

esp_err_t Setting::load() {
    esp_err_t ret = iot_param_load(SETTINGS_NAMESPACE, SETTINGS_KEY, this);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to load settings, error: %s", esp_err_to_name(ret));
        // Initialize with default values
        mode = MODE_MANUAL;
        vol_max = 0;
        vol_min = 0;
        target_pitch = 0;
        target_yaw = 0;
        pos_pid.p = 106;
        pos_pid.i = 16;
        pos_pid.d = 0;
        pos_pid.integral_limit = 400;
        pos_pid.max_out = 500;
        pos_pid.input_max_err = 0;
        pos_pid.Kc = 0.01;

        vel_pid.p = 80;
        vel_pid.i = 3.2;
        vel_pid.d = 0;
        vel_pid.integral_limit = 800;
        vel_pid.max_out = 1000;
        vel_pid.input_max_err = 0;
        vel_pid.Kc = 0.01;
        save();
    }
    return ret;
}

esp_err_t Setting::save() {
    return iot_param_save(SETTINGS_NAMESPACE, SETTINGS_KEY, this, sizeof(Setting));
}
