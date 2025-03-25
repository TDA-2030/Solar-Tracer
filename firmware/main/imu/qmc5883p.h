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

#ifndef HAL_COMPASS_QMC5883P_I2C_ADDR
#define HAL_COMPASS_QMC5883P_I2C_ADDR 0x2C
#endif


class AP_Compass_QMC5883P  {
public:

    AP_Compass_QMC5883P();
    void read();

private:
    i2c_bus_device_handle_t i2c_device;
    int write_register(  int reg, uint8_t value );
    int read_registers(int reg, uint8_t *buffer, int count );

    void _dump_registers();
    bool _check_whoami();
};
