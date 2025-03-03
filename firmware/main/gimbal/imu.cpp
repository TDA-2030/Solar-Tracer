/*
@ link : http://wit-motion.cn


*/

#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wit_c_sdk.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "imu.h"
#include "board.h"
#include "gimbal.h"

#define BUF_SIZE 1024

#define ACC_UPDATE      0x01
#define GYRO_UPDATE     0x02
#define ANGLE_UPDATE    0x04
#define MAG_UPDATE      0x08
#define TEMP_UPDATE     0x10
#define READ_UPDATE     0x80
static volatile char s_cDataUpdate = 0;
static void SensorDataUpdata(uint32_t uiReg, uint32_t uiRegNum);
static void AutoScanSensor(void);
static IMU *globalInstance = nullptr;

#define IMU_UART_NUM UART_NUM_0

static void UsartInit(int baud_rate)
{
    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk =  UART_SCLK_DEFAULT,
        .flags = { .allow_pd = 0, .backup_before_sleep = 0 },
    };

    ESP_ERROR_CHECK(uart_driver_install(IMU_UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(IMU_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(IMU_UART_NUM, UART_PIN_NO_CHANGE, BOARD_IO_GPS_TX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

static void SensorUartSend(uint8_t *p_data, uint32_t uiSize)
{
    uart_write_bytes(IMU_UART_NUM, (const char *)p_data, uiSize);
}

static void Delayms(uint16_t usMs)
{
    vTaskDelay(usMs / portTICK_PERIOD_MS);
}

static void imu_Usart_task(void *pvParameters)
{
    unsigned char ucTemp;
    UsartInit(9600);
    ESP_LOGI("imu", "imu task started");
    while (1) {
        if (uart_read_bytes(IMU_UART_NUM, &ucTemp, 1, portMAX_DELAY) == 1) {
            WitSerialDataIn(ucTemp);
        }
    }
}


IMU::IMU()
{
    ESP_LOGI("imu", "imu initializing");
    WitInit(WIT_PROTOCOL_NORMAL, 0x50);
    WitSerialWriteRegister(SensorUartSend);
    WitRegisterCallBack(SensorDataUpdata);
    WitDelayMsRegister(Delayms);
    xTaskCreate(imu_Usart_task, "imu_task", 4096, NULL, 3, &imuTaskHandle);
    printf("\r\n********************** wit-motion normal example  ************************\r\n");
    AutoScanSensor();
    globalInstance = this;
}

IMU::~IMU()
{
    ESP_LOGW("imu", "imu task deleted");
    vTaskDelete(imuTaskHandle);
}


// 更新传感器数据
static void SensorDataUpdata(uint32_t uiReg, uint32_t uiRegNum)
{
    int i;
    for (i = 0; i < uiRegNum; i++) {
        switch (uiReg) {
//            case AX:
//            case AY:
        case AZ:
            s_cDataUpdate |= ACC_UPDATE;
            break;
//            case GX:
//            case GY:
        case GZ:
            s_cDataUpdate |= GYRO_UPDATE;
            break;
//            case HX:
//            case HY:
        case HZ:
            s_cDataUpdate |= MAG_UPDATE;
            if (globalInstance) {
                imu_data_t& dat = globalInstance->imu_data;
                for (int i = 0; i < 3; i++) {
                    dat.acc.data[i] = sReg[AX + i] / 32768.0f * 16.0f;
                    dat.gyro.data[i] = sReg[GX + i] / 32768.0f * 2000.0f;
                    dat.angle.data[i] = sReg[Roll + i] / 32768.0f * 180.0f;
                }
                if (s_cDataUpdate & ACC_UPDATE) {
                    printf("acc:%.3f %.3f %.3f\r\n", dat.acc.data[0], dat.acc.data[1], dat.acc.data[2]);
                    s_cDataUpdate &= ~ACC_UPDATE;
                }
                if (s_cDataUpdate & GYRO_UPDATE) {
                    printf("gyro:%.3f %.3f %.3f\r\n", dat.gyro.data[0], dat.gyro.data[1], dat.gyro.data[2]);
                    s_cDataUpdate &= ~GYRO_UPDATE;
                }
                if (s_cDataUpdate & ANGLE_UPDATE) {
                    printf("angle:%.3f %.3f %.3f\r\n", dat.angle.data[0], dat.angle.data[1], dat.angle.data[2]);
                    s_cDataUpdate &= ~ANGLE_UPDATE;
                }
                if (s_cDataUpdate & MAG_UPDATE) {
                    printf("mag:%d %d %d\r\n", sReg[HX], sReg[HY], sReg[HZ]);
                    s_cDataUpdate &= ~MAG_UPDATE;
                }
                if (s_cDataUpdate & TEMP_UPDATE) {
                    printf("temp:%f\r\n", sReg[TEMP] / 100.0);
                    s_cDataUpdate &= ~TEMP_UPDATE;
                }
                globalInstance->notifyObservers(dat); // 通知所有观察者

            }
            break;
//            case Roll:
//            case Pitch:
        case Yaw:
            s_cDataUpdate |= ANGLE_UPDATE;
            // xSemaphoreGive(imuSemaphore);
            break;
        case TEMP:
            s_cDataUpdate |= TEMP_UPDATE;
            break;
        default:
            s_cDataUpdate |= READ_UPDATE;
            break;
        }
        uiReg++;
    }
}

static void AutoScanSensor(void)
{
    const uint32_t c_uiBaud[10] = {0, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600};
    int i, iRetry;

    for (i = 1; i < 10; i++) {
        uart_set_baudrate(IMU_UART_NUM, c_uiBaud[i]);
        iRetry = 2;
        do {
            s_cDataUpdate = 0;
            WitReadReg(AX, 3);
            Delayms(100);
            if (s_cDataUpdate != 0) {
                printf("%u baud find sensor\r\n\r\n", c_uiBaud[i]); //lu
                return ;
            }
            iRetry--;
        } while (iRetry);
    }
    printf("can not find sensor\r\n");
}

int IMU::startAccCali()
{
    return WitStartAccCali();
}

int IMU::startMagCali()
{
    return WitStartMagCali();
}

int IMU::stopMagCali()
{
    return WitStopMagCali();
}

int IMU::setBandwidth(int bandwidth)
{
    return WitSetBandwidth(bandwidth);
}

int IMU::setUartBaud(int baud)
{
    return WitSetUartBaud(baud);
}

int IMU::setOutputRate(int rate)
{
    return WitSetOutputRate(rate);
}

int IMU::setContent(int content)
{
    return WitSetContent(content);
}
