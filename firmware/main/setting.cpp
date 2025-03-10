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
    if (ret != ESP_OK || !validateChecksum() || !validateRanges()) {
        ESP_LOGW(TAG, "Failed to load settings or validation failed, error: %s", esp_err_to_name(ret));
        // Initialize with default values
        mode = MODE_MANUAL;
        vol_max = 10;
        vol_min = 13;
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

        pitch_pid.p = 1;
        pitch_pid.i = 0.0;
        pitch_pid.d = 0.0;

        yaw_offset = 0;
        updateChecksum();
        save();
    }
    return ret;
}

esp_err_t Setting::save() {
    if (!validateRanges()) {
        ESP_LOGE(TAG, "Settings validation failed, refusing to save");
        return ESP_ERR_INVALID_STATE;
    }
    updateChecksum();
    return iot_param_save(SETTINGS_NAMESPACE, SETTINGS_KEY, this, sizeof(Setting));
}

bool Setting::validateChecksum() {
    uint32_t calculated = calculateChecksum();
    return calculated == checksum;
}

void Setting::updateChecksum() {
    checksum = calculateChecksum();
}

uint32_t Setting::calculateChecksum() {
    uint32_t sum = 0;
    const uint8_t* data = reinterpret_cast<const uint8_t*>(this);
    // Skip the checksum field itself in the calculation
    for (size_t i = sizeof(checksum); i < sizeof(Setting); i++) {
        sum = (sum << 1) + data[i];
    }
    return sum;
}

bool Setting::validateRanges() {
    // Validate voltage ranges
    if (vol_max <= vol_min || vol_min < 0 || vol_max > 24) {
        return false;
    }

    // Validate PID parameters
    if (pos_pid.p < 0 || pos_pid.i < 0 || pos_pid.d < 0 ||
        vel_pid.p < 0 || vel_pid.i < 0 || vel_pid.d < 0 ||
        pitch_pid.p < 0 || pitch_pid.i < 0 || pitch_pid.d < 0) {
        return false;
    }

    // Validate angle ranges
    if (target_pitch < -90 || target_pitch > 90 ||
        target_yaw < -180 || target_yaw > 180) {
        return false;
    }

    return true;
}
