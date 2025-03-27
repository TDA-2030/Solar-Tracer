/****************************************************************************
 *  Copyright (C) 2020 RoboMaster.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of?
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.? See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 ***************************************************************************/

#ifndef __PID_H__
#define __PID_H__

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline void abs_limit(float *a, float ABS_MAX, float ABS_MIN)
{
    if (*a > ABS_MAX) {
        *a = ABS_MAX;
    }
    if (*a < ABS_MIN) {
        *a = ABS_MIN;
    }
}

struct pid_param {
    float p;
    float i;
    float d;
    float input_max_err;
    float Kc;        // 补偿系数

    float max_out;
    float integral_limit;
};

struct pid {
    struct pid_param *param;

    uint8_t enable;

    float set;
    float get;

    float err;
    float last_err;
    float e_back;

    float pout;
    float iout;
    float dout;
    float out;

};


void pid_struct_init(struct pid *pid, const struct pid_param *_param);

float pid_calculate(struct pid *pid, float get, float set, float dt); // dt单位为s

#ifdef __cplusplus
}
#endif

#endif // __PID_H__