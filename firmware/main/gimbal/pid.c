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
#include <math.h>
#include "pid.h"

static inline void abs_limit(float *a, float ABS_MAX, float ABS_MIN)
{
    if (*a > ABS_MAX) {
        *a = ABS_MAX;
    }
    if (*a < ABS_MIN) {
        *a = ABS_MIN;
    }
}

/**
  * @brief     calculate delta PID and position PID
  * @param[in] pid: control pid struct
  * @param[in] get: measure feedback value
  * @param[in] set: target value
  * @retval    pid calculate output
  */
float pid_calculate(struct pid *pid, float get, float set, float dt)
{
    pid->get = get;
    pid->set = set;
    pid->err = set - get;
    if ((pid->param.input_max_err != 0) && (fabs(pid->err) > pid->param.input_max_err)) {
        return 0;
    }

    pid->pout = pid->param.p * pid->err;
    pid->iout += pid->param.i * pid->err * dt;
    pid->dout = pid->param.d * (pid->err - pid->last_err) / dt;

    pid->out = pid->pout + pid->iout + pid->dout;
    float u_unlimited = pid->out;
    abs_limit(&(pid->iout), pid->param.integral_limit, -pid->param.integral_limit);
    abs_limit(&(pid->out), pid->param.max_out, -pid->param.max_out);
    // 反向计算修正积分项
    pid->e_back = (pid->out - u_unlimited) * pid->param.Kc;
    pid->iout += ( pid->e_back) * dt;  // 修正积分累积

    if (pid->enable == 0) {
        pid->out = 0;
    }

    return pid->out;
}

/**
  * @brief     initialize pid parameter
  * @retval    none
  */
void pid_struct_init(struct pid *pid, const struct pid_param *_param)
{
    pid->enable = 1;

    pid->param = *_param;
    pid->err = 0;
    pid->last_err = 0;
    
    pid->pout = 0;
    pid->iout = 0;
    pid->dout = 0;
    pid->out = 0;
}
