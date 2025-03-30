

#include <string.h>
#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "imu_bmi270.h"
#include "board.h"

#include "app_datafusion.h"
#include "common/common.h"

static const char *TAG = "imu-bmi270";

#define GRAVITY_EARTH       (9.80665f)
#define ACCEL               UINT8_C(0x00)
#define GYRO                UINT8_C(0x01)

static IMUBmi270 *globalInstance = nullptr;


/*!
 * @brief This function converts lsb to meter per second squared for 16 bit accelerometer at
 * range 2G, 4G, 8G or 16G.
 */
static float lsb_to_mps2(int16_t val, float g_range, uint8_t bit_width)
{
    double power = 2;

    float half_scale = (float)((pow((double)power, (double)bit_width) / 2.0f));

    return (GRAVITY_EARTH * val * g_range) / half_scale;
}

/*!
 * @brief This function converts lsb to degree per second for 16 bit gyro at
 * range 125, 250, 500, 1000 or 2000dps.
 */
static float lsb_to_dps(int16_t val, float dps, uint8_t bit_width)
{
    double power = 2;

    float half_scale = (float)((pow((double)power, (double)bit_width) / 2.0f));

    return (dps / (half_scale)) * (val);
}


static int8_t set_accel_gyro_config(struct bmi2_dev *bmi)
{

    int8_t rslt;
    struct bmi2_sens_config config[2];

    config[ACCEL].type = BMI2_ACCEL;
    config[GYRO].type = BMI2_GYRO;

    rslt = bmi2_get_sensor_config(config, 2, bmi);
    bmi2_error_codes_print_result(rslt);

    rslt = bmi2_map_data_int(BMI2_DRDY_INT, BMI2_INT1, bmi);
    bmi2_error_codes_print_result(rslt);

    if (rslt == BMI2_OK) {

        config[ACCEL].cfg.acc.odr = BMI2_ACC_ODR_200HZ;
        config[ACCEL].cfg.acc.range = BMI2_ACC_RANGE_2G;
        config[ACCEL].cfg.acc.bwp = BMI2_ACC_NORMAL_AVG4;
        config[ACCEL].cfg.acc.filter_perf = BMI2_PERF_OPT_MODE;
        config[GYRO].cfg.gyr.odr = BMI2_GYR_ODR_200HZ;
        config[GYRO].cfg.gyr.range = BMI2_GYR_RANGE_2000;
        config[GYRO].cfg.gyr.bwp = BMI2_GYR_NORMAL_MODE;
        config[GYRO].cfg.gyr.noise_perf = BMI2_POWER_OPT_MODE;
        config[GYRO].cfg.gyr.filter_perf = BMI2_PERF_OPT_MODE;

        rslt = bmi2_set_sensor_config(config, 2, bmi);
        bmi2_error_codes_print_result(rslt);
    }

    return rslt;
}

static void bmi270_enable_accel_gyro(struct bmi2_dev *bmi2_dev)
{
    int8_t rslt;

    /* Assign accel and gyro sensor to variable. */
    uint8_t sensor_list[3] = { BMI2_ACCEL, BMI2_GYRO, BMI2_TEMP };

    struct bmi2_sens_config config;
    /* Accel and gyro configuration settings. */
    rslt = set_accel_gyro_config(bmi2_dev);
    bmi2_error_codes_print_result(rslt);

    if (rslt == BMI2_OK) {
        rslt = bmi2_sensor_enable(sensor_list, 2, bmi2_dev);
        bmi2_error_codes_print_result(rslt);

        if (rslt == BMI2_OK) {
            config.type = BMI2_ACCEL;
            rslt = bmi2_get_sensor_config(&config, 1, bmi2_dev);
            bmi2_error_codes_print_result(rslt);
        }
    }
}

float IMUBmi270::readTemperature()
{
    int8_t rslt;
    struct bmi2_dev *bmi2_dev = bmi_handle;
    uint16_t temperature_data;
    rslt = bmi2_get_temperature_data(&temperature_data, bmi2_dev);
    bmi2_error_codes_print_result(rslt);

    float temperature_value = (float)((((float)((int16_t)temperature_data)) / 512.0) + 23.0);
    return temperature_value;
}

void IMUBmi270::readData()
{
    struct bmi2_dev *bmi2_dev = globalInstance->bmi_handle;
    imu_data_t &_data = globalInstance->imu_data;
    int8_t rslt;
    struct bmi2_sens_data sensor_data;
    rslt = bmi2_get_sensor_data(&sensor_data, bmi2_dev);
    bmi2_error_codes_print_result(rslt);
    _data.temperature = readTemperature();

    if ((rslt == BMI2_OK) && (sensor_data.status & BMI2_DRDY_ACC) && (sensor_data.status & BMI2_DRDY_GYR)) {
        /* Converting lsb to meter per second squared for 16 bit accelerometer at 2G range. */
        _data.acc.x = lsb_to_mps2(sensor_data.acc.x, (float)2, bmi2_dev->resolution);
        _data.acc.y = lsb_to_mps2(sensor_data.acc.y, (float)2, bmi2_dev->resolution);
        _data.acc.z = lsb_to_mps2(sensor_data.acc.z, (float)2, bmi2_dev->resolution);

        /* Converting lsb to degree per second for 16 bit gyro at 2000dps range. */
        _data.gyro.x = lsb_to_dps(sensor_data.gyr.x, (float)2000, bmi2_dev->resolution);
        _data.gyro.y = lsb_to_dps(sensor_data.gyr.y, (float)2000, bmi2_dev->resolution);
        _data.gyro.z = lsb_to_dps(sensor_data.gyr.z, (float)2000, bmi2_dev->resolution);
        calculateAttitude(&_data, 0.01f);
        notifyObservers(_data);
    } else {
        ESP_LOGW(TAG, "Sensor data not ready");
    }

}

static void imu_task(void *arg)
{
    vTaskDelay(pdMS_TO_TICKS(500));
    while (1) {
        globalInstance->readData();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

int IMUBmi270::init()
{

    i2c_bus_handle_t i2c_bus_handle = bsp_i2c_get_handle();
    if (!i2c_bus_handle) {
        ESP_LOGE(TAG, "Failed to get i2c bus handle");
        return -1;
    }

    bmi270_i2c_config_t i2c_bmi270_conf = {
        .i2c_handle = i2c_bus_handle,
        .i2c_addr = BMI270_I2C_ADDRESS,
    };

    esp_err_t ret = bmi270_sensor_create(&i2c_bmi270_conf, &bmi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create bmi270 sensor");
        return -1;
    }
    globalInstance = this;
    bmi270_enable_accel_gyro(bmi_handle);

    BaseType_t res;
    res = xTaskCreate(imu_task, "imu_task", 4096, NULL, configMAX_PRIORITIES - 1, &imuTaskHandle);
    if (res != pdPASS) {
        ESP_LOGE(TAG, "Create imu task fail!");
        return -1;
    }
    return 0;
}

IMUBmi270::IMUBmi270(): bmi_handle(nullptr)
{

}

IMUBmi270::~IMUBmi270()
{

}
