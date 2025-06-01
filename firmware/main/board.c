#include "board.h"

static bool i2c_initialized = false;
static i2c_bus_handle_t i2c_bus_handle = NULL;
static i2c_bus_handle_t i2c_bus_handle1 = NULL;

esp_err_t bsp_i2c_init(void)
{
    /* I2C was initialized before */
    if (i2c_initialized) {
        return ESP_OK;
    }

    i2c_config_t i2c_bus_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BOARD_IO_MOTY_ENC_A,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = BOARD_IO_MOTY_ENC_B,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000
    };
    i2c_bus_handle = i2c_bus_create(I2C_NUM_0, &i2c_bus_conf);

    i2c_bus_conf.sda_io_num = BOARD_IO_IMU_SDA;
    i2c_bus_conf.scl_io_num = BOARD_IO_IMU_SCL;
    i2c_bus_handle1 = i2c_bus_create(I2C_NUM_1, &i2c_bus_conf);

    i2c_initialized = true;

    return ESP_OK;
}

esp_err_t bsp_i2c_deinit(void)
{
    i2c_bus_delete(&i2c_bus_handle);
    i2c_bus_delete(&i2c_bus_handle1);
    i2c_initialized = false;
    return ESP_OK;
}

i2c_bus_handle_t bsp_i2c_get_handle(int index)
{
    return (index == 0) ? i2c_bus_handle : i2c_bus_handle1;
}

// i2cconfig --port=0 --sda=17 --scl=18 --freq=100000
// i2cconfig --port=0 --sda=18 --scl=17 --freq=100000
// i2cdetect