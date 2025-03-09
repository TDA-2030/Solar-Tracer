/*
@ link : http://wit-motion.cn


*/

#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wit_c_sdk.h"
#include "driver/uart.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "imu.h"
#include "board.h"
#include "gimbal.h"

static const char *TAG = "imu";

#define IMU_USE_UART 0

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

#if IMU_USE_UART
#define IMU_UART_NUM UART_NUM_1

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
    ESP_ERROR_CHECK(uart_set_pin(IMU_UART_NUM, UART_PIN_NO_CHANGE, BOARD_IO_IMU_TX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

static void SensorUartSend(uint8_t *p_data, uint32_t uiSize)
{
    uart_write_bytes(IMU_UART_NUM, (const char *)p_data, uiSize);
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
            // WitReadReg(AX, 3);
            Delayms(100);
            if (s_cDataUpdate != 0) {
                ESP_LOGI(TAG, "IMU Sensor at %u baud\r\n", c_uiBaud[i]); //lu
                return ;
            }
            iRetry--;
        } while (iRetry);
    }
    ESP_LOGE(TAG, "can not find sensor");
}
#else

#define ACK_CHECK_EN 0x1                        /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0                       /*!< I2C master will not check ack from slave */
#define I2C_MASTER_TX_BUF_DISABLE 0             /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0             /*!< I2C master doesn't need buffer */
static i2c_port_t i2c_master_port = I2C_NUM_0;

static esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BOARD_IO_IMU_SDA,
        .scl_io_num = BOARD_IO_IMU_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = {
            .clk_speed = 400000,
        },
        .clk_flags = 0,          /*!< Optional, you can use I2C_SCLK_SRC_FLAG_* flags to choose i2c source clock here. */
    };
    esp_err_t err = i2c_param_config(i2c_master_port, &conf);
    if (err != ESP_OK) {
        return err;
    }
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}


static int32_t WitIICRead(uint8_t ucAddr, uint8_t ucReg, uint8_t *p_ucVal, uint32_t uiLen)
{
    int ret;
    int i;

    if (uiLen == 0) {
        return 0;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ucAddr, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, ucReg, ACK_CHECK_EN);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ucAddr | 0x01, ACK_CHECK_EN);
    for (i = 0; i < uiLen; i++) {
        if (i == uiLen - 1) { // last pack
            i2c_master_read_byte(cmd, p_ucVal + i, I2C_MASTER_NACK);
        } else {
            i2c_master_read_byte(cmd, p_ucVal + i, I2C_MASTER_ACK);
        }
    }
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_master_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

static int32_t WitIICWrite(uint8_t ucAddr, uint8_t ucReg, uint8_t *p_ucVal, uint32_t uiLen)
{
    int ret;
    int i;

    if (uiLen == 0) {
        return 0;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ucAddr, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, ucReg, ACK_CHECK_EN);
    for (i = 0; i < uiLen; i++) {
        if (i == uiLen - 1) { // last pack
            i2c_master_read_byte(cmd, p_ucVal + i, I2C_MASTER_NACK);
        } else {
            i2c_master_read_byte(cmd, p_ucVal + i, I2C_MASTER_ACK);
        }
    }
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_master_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

#endif


static void Delayms(uint16_t usMs)
{
    vTaskDelay(usMs / portTICK_PERIOD_MS);
}


static void imu_task(void *pvParameters)
{
#if IMU_USE_UART
    unsigned char ucTemp;
    UsartInit(9600);
#else
    i2c_master_init();
#endif

    ESP_LOGI(TAG, "imu task started");
    while (1) {
#if IMU_USE_UART
        if (uart_read_bytes(IMU_UART_NUM, &ucTemp, 1, portMAX_DELAY) == 1) {
            WitSerialDataIn(ucTemp);
        }
#else
        int ret = WitReadReg(AX, 13); // 读取传感器数据
        if (ret != WIT_HAL_OK) {
            ESP_LOGE(TAG, "imu read error (%d)", ret);
        }
        Delayms(20);
#endif
    }
}

IMU::IMU()
{
    ESP_LOGI(TAG, "imu initializing");
#if IMU_USE_UART
    WitInit(WIT_PROTOCOL_NORMAL, 0x50);
#else
    WitInit(WIT_PROTOCOL_I2C, 0x50);
#endif
    WitRegisterCallBack(SensorDataUpdata);
    WitDelayMsRegister(Delayms);
#if IMU_USE_UART
    WitSerialWriteRegister(SensorUartSend);
#else
    WitI2cFuncRegister(WitIICWrite, WitIICRead);
#endif
    xTaskCreate(imu_task, "imu_task", 4096, NULL, configMAX_PRIORITIES - 1, &imuTaskHandle);
    globalInstance = this;
}

IMU::~IMU()
{
    ESP_LOGW(TAG, "imu task deleted");
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
                imu_data_t &dat = globalInstance->imu_data;
                for (int i = 0; i < 3; i++) {
                    dat.acc.data[i] = sReg[AX + i] / 32768.0f * 16.0f;
                    dat.gyro.data[i] = sReg[GX + i] / 32768.0f * 2000.0f;
                    dat.angle.data[i] = sReg[Roll + i] / 32768.0f * 180.0f;
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
