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
#pragma once

#include "i2c_bus.h"
#include "imu_base.h"

#ifndef HAL_COMPASS_QMC5883P_I2C_ADDR
#define HAL_COMPASS_QMC5883P_I2C_ADDR 0x2C
#endif


class AP_Compass_QMC5883P  {
public:

    AP_Compass_QMC5883P();
    void read();
    int getAzimuth();
    void setMagneticDeclination(int degrees, uint8_t minutes);
    void setMagneticDeclination(float degrees)
    {
        _magneticDeclinationDegrees = degrees;
    }

private:
    i2c_bus_device_handle_t i2c_device;
    int write_register(  int reg, uint8_t value );
    int read_registers(int reg, uint8_t *buffer, int count );
    void _applyCalibration();
    void calibrate();
    void clearCalibration() {
        setCalibrationOffsets(0., 0., 0.);
	    setCalibrationScales(1., 1., 1.);
    }

    void setCalibrationOffsets(float x_offset, float y_offset, float z_offset) {
        _offset.x = x_offset;
        _offset.y = y_offset;
        _offset.z = z_offset;
    }

    void setCalibrationScales(float x_scale, float y_scale, float z_scale) {
        _scale.x = x_scale;
        _scale.y = y_scale;
        _scale.z = z_scale;
    }
    void setCalibration(int x_min, int x_max, int y_min, int y_max, int z_min, int z_max);


    void _dump_registers();
    bool _check_whoami();
	axis_t _vRaw, _vCalibrated;
	axis_t _offset = {0, 0, 0};
    axis_t _scale = {1.0f, 1.0f, 1.0f};
    float _magneticDeclinationDegrees = 0.0f;

};
