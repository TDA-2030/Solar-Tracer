#include <string.h>
#include "esp_log.h"
#include "setting.h"
#include "helper.h"

static const char *TAG = "setting";

Setting g_settings;

Setting::Setting()
{

}

void Setting::restortDefault()
{
    ESP_LOGI(TAG, "restortDefault");
    mode = MODE_MANUAL;
    vol_max = 13;
    vol_min = 10;
    target_pitch = 0;
    target_yaw = 0;
    yaw_offset = 0;
    pos_pid.p = 112;
    pos_pid.i = 700;
    pos_pid.d = 0;
    pos_pid.integral_limit = 500;
    pos_pid.max_out = 220;
    pos_pid.input_max_err = 0;
    pos_pid.Kc = 0.01;

    vel_pid.p = 8.0f;
    vel_pid.i = 80.0f;
    vel_pid.d = 0;
    vel_pid.integral_limit = 1000;
    vel_pid.max_out = 1000;
    vel_pid.input_max_err = 0;
    vel_pid.Kc = 0.01;

    pitch_vel_pid.p = 98.0f;
    pitch_vel_pid.i = 30.0f;
    pitch_vel_pid.d = 0;
    pitch_vel_pid.integral_limit = 550;
    pitch_vel_pid.max_out = 1000;
    pitch_vel_pid.input_max_err = 0;
    pitch_vel_pid.Kc = 0.01;

    pitch_pid.p = 2.0f;
    pitch_pid.i = 4.0f;
    pitch_pid.d = 0.0;
    pitch_pid.integral_limit = 700;
    pitch_pid.max_out = 1000;
}

void Setting::print()
{
    ESP_LOGI(TAG, "Settings:");
    ESP_LOGI(TAG, "mode: %d", mode);
    ESP_LOGI(TAG, "vol_max: %f", vol_max);
    ESP_LOGI(TAG, "vol_min: %f", vol_min);
    ESP_LOGI(TAG, "target_pitch: %f", target_pitch);
    ESP_LOGI(TAG, "target_yaw: %f", target_yaw);
    ESP_LOGI(TAG, "yaw_offset: %f", yaw_offset);
    ESP_LOGI(TAG, "pos_pid.p: %f", pos_pid.p);
    ESP_LOGI(TAG, "pos_pid.i: %f", pos_pid.i);
    ESP_LOGI(TAG, "pos_pid.d: %f", pos_pid.d);
    ESP_LOGI(TAG, "pos_pid.integral_limit: %f", pos_pid.integral_limit);
    ESP_LOGI(TAG, "pos_pid.max_out: %f", pos_pid.max_out);
    ESP_LOGI(TAG, "pos_pid.input_max_err: %f", pos_pid.input_max_err);
    ESP_LOGI(TAG, "pos_pid.Kc: %f", pos_pid.Kc);
    ESP_LOGI(TAG, "vel_pid.p: %f", vel_pid.p);
    ESP_LOGI(TAG, "vel_pid.i: %f", vel_pid.i);
    ESP_LOGI(TAG, "vel_pid.d: %f", vel_pid.d);
    ESP_LOGI(TAG, "vel_pid.integral_limit: %f", vel_pid.integral_limit);
    ESP_LOGI(TAG, "vel_pid.max_out: %f", vel_pid.max_out);
    ESP_LOGI(TAG, "vel_pid.input_max_err: %f", vel_pid.input_max_err);
    ESP_LOGI(TAG, "vel_pid.Kc: %f", vel_pid.Kc);
    ESP_LOGI(TAG, "pitch_pid.p: %f", pitch_pid.p);
    ESP_LOGI(TAG, "pitch_pid.i: %f", pitch_pid.i);
    ESP_LOGI(TAG, "pitch_pid.d: %f", pitch_pid.d);
    ESP_LOGI(TAG, "pitch_pid.integral_limit: %f", pitch_pid.integral_limit);
    ESP_LOGI(TAG, "pitch_pid.max_out: %f", pitch_pid.max_out);
    ESP_LOGI(TAG, "pitch_pid.input_max_err: %f", pitch_pid.input_max_err);
    ESP_LOGI(TAG, "pitch_pid.Kc: %f", pitch_pid.Kc);
    ESP_LOGI(TAG, "checksum: %u", checksum);
}

esp_err_t Setting::load()
{
    esp_err_t ret = iot_param_load(SETTINGS_NAMESPACE, SETTINGS_KEY, this);
    if (ret != ESP_OK || !validateChecksum() || !validateRanges()) {
        ESP_LOGW(TAG, "Failed to load settings or validation failed, error: %s", esp_err_to_name(ret));
        // Initialize with default values
        restortDefault();
        updateChecksum();
        save();
    }
    print();

    return ret;
}

esp_err_t Setting::save()
{
    if (!validateRanges()) {
        ESP_LOGE(TAG, "Settings validation failed, refusing to save");
        return ESP_ERR_INVALID_STATE;
    }
    updateChecksum();
    return iot_param_save(SETTINGS_NAMESPACE, SETTINGS_KEY, this, sizeof(Setting));
}

bool Setting::validateChecksum()
{
    uint32_t calculated = calculateChecksum();
    return 1;//calculated == checksum;
}

void Setting::updateChecksum()
{
    checksum = calculateChecksum();
}

uint32_t Setting::calculateChecksum()
{
    uint32_t sum = 0;
    const uint8_t *data = reinterpret_cast<const uint8_t *>(this);
    // Skip the checksum field itself in the calculation
    for (size_t i = sizeof(checksum); i < sizeof(Setting); i++) {
        sum = (sum << 1) + data[i];
    }
    return sum;
}

bool Setting::validateRanges()
{
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
