/*
 * This file is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Driver by Lokesh Ramina, Jan 2022
 */
#include <stdio.h>
#include "esp_log.h"
#include "qmc5883p.h"
#include "board.h"

static const char *TAG = "qmc5883p";

#if 1
#define HAL_COMPASS_QMC5883P_I2C_ADDR 0x2C

//Register Address
#define QMC5883P_REG_ID                 0x00
#define QMC5883P_REG_DATA_OUTPUT_X      0x01
#define QMC5883P_REG_DATA_OUTPUT_Z_MSB  0x06
#define QMC5883P_REG_STATUS             0x09
#define QMC5883P_REG_CONF1              0x0A
#define QMC5883P_REG_CONF2              0x0B

#define QMC5883P_ID_VAL 0x80

//Register values
// Sensor operation modes
#define QMC5883P_MODE_SUSPEND    0x00
#define QMC5883P_MODE_NORMAL     0x01
#define QMC5883P_MODE_SINGLE     0x02
#define QMC5883P_MODE_CONTINUOUS 0x03

// ODR data output rates for 5883L
#define QMC5883P_ODR_10HZ  (0x00 << 2)
#define QMC5883P_ODR_50HZ  (0x01 << 2)
#define QMC5883P_ODR_100HZ (0x02 << 2)
#define QMC5883P_ODR_200HZ (0x03 << 2)

// Over sampling Ratio OSR1
#define QMC5883P_OSR1_8 (0x00 << 4)
#define QMC5883P_OSR1_4 (0x01 << 4)
#define QMC5883P_OSR1_2 (0x02 << 4)
#define QMC5883P_OSR1_1 (0x03 << 4)

// Down sampling Rate OSR2
#define QMC5883P_OSR2_8 0x08

//RNG
#define QMC5883P_RNG_30G (0x00 << 2)
#define QMC5883P_RNG_12G (0x01 << 2)
#define QMC5883P_RNG_8G  (0x10 << 2)
#define QMC5883P_RNG_2G  (0x11 << 2)

#define QMC5883P_SET_XYZ_SIGN 0x29

//Reset
#define QMC5883P_RST 0x80

//Status Val
#define QMC5883P_STATUS_DATA_READY 0x01

#ifndef DEBUG
#define DEBUG 0
#endif


int AP_Compass_QMC5883P::write_register(  int reg, uint8_t value )
{
    esp_err_t ret = i2c_bus_write_bytes(i2c_device, reg, 1, &value);
    return 1 ? ret == ESP_OK : 0;
}

int AP_Compass_QMC5883P::read_registers(int reg, uint8_t *buffer, int count )
{
    esp_err_t ret = i2c_bus_read_bytes(i2c_device, reg, count, buffer);

    return 1 ? ret == ESP_OK : 0;
}

AP_Compass_QMC5883P::AP_Compass_QMC5883P()
{
}

bool AP_Compass_QMC5883P::init()
{
    i2c_bus_handle_t i2c0_bus = bsp_i2c_get_handle();
    if (!i2c0_bus) {
        ESP_LOGE(TAG, "Failed to get i2c bus handle");
        return false;
    }

    i2c_device = i2c_bus_device_create(i2c0_bus, HAL_COMPASS_QMC5883P_I2C_ADDR, 0);

#if DEBUG
    _dump_registers();
#endif
    if (!_check_whoami()) {
        goto fail;
    }
    //As mentioned in the Datasheet 7.2 to do continues mode 0x29 will set sign for X,Y,Z
    if (!write_register(QMC5883P_REG_DATA_OUTPUT_Z_MSB, QMC5883P_SET_XYZ_SIGN) ||
            !write_register(QMC5883P_REG_CONF1,
                            QMC5883P_MODE_CONTINUOUS |
                            QMC5883P_ODR_100HZ |
                            QMC5883P_OSR1_8 |
                            QMC5883P_OSR2_8) ||
            !write_register(QMC5883P_REG_CONF2, QMC5883P_OSR2_8)) {
        goto fail;
    }

    return true;

fail:
    return false;
}

bool AP_Compass_QMC5883P::_check_whoami()
{
    uint8_t whoami;
    if (!read_registers(QMC5883P_REG_ID, &whoami, 1) ||
            whoami != QMC5883P_ID_VAL) {
        return false;
    }
    return true;
}

void AP_Compass_QMC5883P::read()
{
    struct PACKED {
        int16_t rx;
        int16_t ry;
        int16_t rz;
    } buffer;

    const float range_scale = 1000.0f / 3000.0f;

    uint8_t status;
    if (!read_registers(QMC5883P_REG_STATUS, &status, 1)) {
        return;
    }
    //new data is ready
    if (!(status & QMC5883P_STATUS_DATA_READY)) {
        printf("no data ready\n");
        return;
    }

    if (!read_registers(QMC5883P_REG_DATA_OUTPUT_X, (uint8_t *) &buffer, sizeof(buffer))) {
        return ;
    }

    auto x = buffer.rx;
    auto y = buffer.ry;
    auto z = buffer.rz;

#if DEBUG
    printf("mag.x:%d\n", x);
    printf("mag.y:%d\n", y);
    printf("mag.z:%d\n", z);
#endif

    // Vector3f field = Vector3f{x * range_scale, y * range_scale, z * range_scale };

    // accumulate_sample(field, _instance, 20);
}



void AP_Compass_QMC5883P::_dump_registers()
{
    printf("QMC5883P registers dump\n");
    for (uint8_t reg = QMC5883P_REG_DATA_OUTPUT_X; reg <= 0x30; reg++) {
        uint8_t v;
        read_registers(reg, &v, 1);
        printf("%02x:%02x ", (unsigned)reg, (unsigned)v);
        if ((reg - ( QMC5883P_REG_DATA_OUTPUT_X - 1)) % 16 == 0) {
            printf("\n");
        }
    }
}

#endif //AP_COMPASS_QMC5883P_ENABLED