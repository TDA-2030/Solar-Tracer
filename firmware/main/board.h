/*

  */
#ifndef __BOARD_H_
#define __BOARD_H_

#include "i2c_bus.h"


#ifdef __cplusplus
extern "C" {
#endif

#define BOARD_IO_MOTY_IN1 15
#define BOARD_IO_MOTY_IN2 16
#define BOARD_IO_MOTY_ENC_A 17
#define BOARD_IO_MOTY_ENC_B 18


#define BOARD_IO_MOTX_IN1 6
#define BOARD_IO_MOTX_IN2 7
#define BOARD_IO_MOTX_ENC_A 5
#define BOARD_IO_MOTX_ENC_B 4


#define BOARD_IO_GPS_RX 35
#define BOARD_IO_GPS_TX 36

#define BOARD_IO_IMU_SDA 38
#define BOARD_IO_IMU_SCL 37

#define BOARD_IO_LED_RED 42
#define BOARD_IO_LED_GREEN 41


esp_err_t bsp_i2c_init(void);
i2c_bus_handle_t bsp_i2c_get_handle(void);


#ifdef __cplusplus
}
#endif

#endif
