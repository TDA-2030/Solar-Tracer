#ifndef _APP_DATAFUSION_H_
#define _APP_DATAFUSION_H_

#include "imu_base.h"

#ifdef __cplusplus
extern "C" {
#endif

void datafusion_update(imu_data_t * imu_data, float dt);


#ifdef __cplusplus
}
#endif

#endif