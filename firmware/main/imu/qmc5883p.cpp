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
#include <math.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "qmc5883p.h"
#include "board.h"

static const char *TAG = "qmc5883p";

#define PI         3.1415926535897932384626433832795

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
#define QMC5883P_STATUS_DATA_OVFL 0x02

#ifndef DEBUG
#define DEBUG 0
#endif


int AP_Compass_QMC5883P::write_register(  int reg, uint8_t value )
{
    esp_err_t ret = i2c_bus_write_bytes(i2c_device, reg, 1, &value);
    return ret == ESP_OK ? 0 : 1;
}

int AP_Compass_QMC5883P::read_registers(int reg, uint8_t *buffer, int count )
{
    esp_err_t ret = i2c_bus_read_bytes(i2c_device, reg, count, buffer);
    return ret == ESP_OK ? 0 : 1;
}

AP_Compass_QMC5883P::AP_Compass_QMC5883P()
{
    int ret=0;
    i2c_bus_handle_t i2c_bus = bsp_i2c_get_handle(1);
    if (!i2c_bus) {
        ESP_LOGE(TAG, "Failed to get i2c bus handle");
        return ;
    }

    i2c_device = i2c_bus_device_create(i2c_bus, HAL_COMPASS_QMC5883P_I2C_ADDR, 0);
    if (!_check_whoami()) {
        ESP_LOGE(TAG, "QMC5883P not found");
        goto fail;
    }

    ret |= write_register(QMC5883P_REG_CONF1,
                            QMC5883P_MODE_CONTINUOUS |
                            QMC5883P_ODR_100HZ |
                            QMC5883P_OSR1_8 |
                            QMC5883P_OSR2_8);
    ret |= write_register( QMC5883P_REG_CONF2, QMC5883P_RNG_12G );
    if (ret) {
        ESP_LOGE(TAG, "QMC5883P init failed");
        goto fail;
    }

#if DEBUG
    _dump_registers();
#endif

fail:
    return ;
}

bool AP_Compass_QMC5883P::_check_whoami()
{
    uint8_t whoami=0;
    read_registers(QMC5883P_REG_ID, &whoami, 1);
    if (whoami != QMC5883P_ID_VAL) {
        ESP_LOGE(TAG, "whoami:0x%02x\n", (unsigned)whoami);
        return false;
    }
    return true;
}

/**
 * Define the magnetic declination for accurate degrees.
 * https://www.magnetic-declination.com/
 * 
 * @example
 * For: Londrina, PR, Brazil at date 2022-12-05
 * The magnetic declination is: -19º 43'
 * 
 * then: setMagneticDeclination(-19, 43);
 */
void AP_Compass_QMC5883P::setMagneticDeclination(int degrees, uint8_t minutes)
{
	_magneticDeclinationDegrees = (float)degrees + (float)minutes / 60.0f;
}

void AP_Compass_QMC5883P::_applyCalibration()
{
	_vCalibrated.x = (_vRaw.x - _offset.x) * _scale.x;
	_vCalibrated.y = (_vRaw.y - _offset.y) * _scale.y;
	_vCalibrated.z = (_vRaw.z - _offset.z) * _scale.z;
}

void AP_Compass_QMC5883P::calibrate() {
	clearCalibration();
	int32_t calibrationData[3][2] = {{65000, -65000}, {65000, -65000}, {65000, -65000}};
  	int32_t	x, y, z;

	uint64_t end_time = esp_timer_get_time() + 10000000; // 10 seconds later
	while(esp_timer_get_time() < end_time) {
		read();

  		x = _vRaw.x;
  		y = _vRaw.y;
  		z = _vRaw.z;

		if(x < calibrationData[0][0]) {
			calibrationData[0][0] = x;
		}
		if(x > calibrationData[0][1]) {
			calibrationData[0][1] = x;
		}

		if(y < calibrationData[1][0]) {
			calibrationData[1][0] = y;
		}
		if(y > calibrationData[1][1]) {
			calibrationData[1][1] = y;
		}

		if(z < calibrationData[2][0]) {
			calibrationData[2][0] = z;
		}
		if(z > calibrationData[2][1]) {
			calibrationData[2][1] = z;
		}
	}

	setCalibration(
		calibrationData[0][0],
		calibrationData[0][1],
		calibrationData[1][0],
		calibrationData[1][1],
		calibrationData[2][0],
		calibrationData[2][1]
	);
}

/**
    SET CALIBRATION
	Set calibration values for more accurate readings
		
	@author Claus Näveke - TheNitek [https://github.com/TheNitek]
	
	@since v1.1.0

	@deprecated Instead of setCalibration, use the calibration offset and scale methods.
**/
void AP_Compass_QMC5883P::setCalibration(int x_min, int x_max, int y_min, int y_max, int z_min, int z_max){
	setCalibrationOffsets(
		(x_min + x_max)/2,
		(y_min + y_max)/2,
		(z_min + z_max)/2
	);

	float x_avg_delta = (x_max - x_min)/2;
	float y_avg_delta = (y_max - y_min)/2;
	float z_avg_delta = (z_max - z_min)/2;

	float avg_delta = (x_avg_delta + y_avg_delta + z_avg_delta) / 3;

	setCalibrationScales(
		avg_delta / x_avg_delta,
		avg_delta / y_avg_delta,
		avg_delta / z_avg_delta
	);
}

void AP_Compass_QMC5883P::read()
{
    struct PACKED {
        int16_t rx;
        int16_t ry;
        int16_t rz;
    };
    struct PACKED buffer;

    uint8_t status;
    if (read_registers(QMC5883P_REG_STATUS, &status, 1)) {
        return;
    }
    //new data is ready
    if (!(status & QMC5883P_STATUS_DATA_READY)) {
        ESP_LOGW(TAG, "no data ready");
        return;
    }
    if (status & QMC5883P_STATUS_DATA_OVFL) {
        ESP_LOGW(TAG, "data overflow");
    }

    if (read_registers(QMC5883P_REG_DATA_OUTPUT_X, (uint8_t *)&buffer, sizeof(buffer))) {
        return ;
    }

    #define FACTOR 24 / 600 // convert to uT in ±12G range
    _vRaw.x  = (float)buffer.rx * FACTOR;
    _vRaw.y = (float)buffer.ry * FACTOR;
    _vRaw.z = (float)buffer.rz * FACTOR;
    _applyCalibration();
    #undef FACTOR

#if DEBUG
    printf("mag:%.2f, %.2f, %.2f\n", _vRaw.x, _vRaw.y, _vRaw.z);
#endif
}

int AP_Compass_QMC5883P::getAzimuth()
{
	float heading = atan2( _vRaw.y, _vRaw.x ) * 180.0 / PI;
	heading += _magneticDeclinationDegrees;
    heading = fmod(heading + 540, 360) - 180;  // 标准化到 -180 到 +180
	return (int)heading;
}

void AP_Compass_QMC5883P::_dump_registers()
{
    printf("QMC5883P registers dump\n");
    const uint8_t start_reg = 0x00;
    for (uint8_t reg = start_reg; reg <= 0x30; reg++) {
        uint8_t v;
        read_registers(reg, &v, 1);
        printf("%02x:%02x ", (unsigned)reg, (unsigned)v);
        if ((reg - ( start_reg - 1)) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}
