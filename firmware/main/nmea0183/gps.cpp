
#include "nmea_parser.h"
#include "esp_log.h"
#include "gps.h"
#include "board.h"

static const char *TAG = "GPS";

static GPS *globalInstance;

/**
 * @brief GPS Event Handler
 *
 * @param event_handler_arg handler specific arguments
 * @param event_base event base, here is fixed to ESP_NMEA_EVENT
 * @param event_id event id
 * @param event_data event specific arguments
 */
static void gps_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
#define TIME_ZONE (+8)   //Beijing Time
#define YEAR_BASE (2000) //date in GPS starts from 2000

    gps_t *gps = NULL;
    switch (event_id) {
    case GPS_UPDATE: {
        gps = (gps_t *)event_data;
        /* print information parsed from GPS statements */
        

        globalInstance->notifyObservers(*gps);
    } break;
    case GPS_UNKNOWN:
        /* print unknown statements */
        ESP_LOGW(TAG, "Unknown statement:%s", (char *)event_data);
        break;
    default:
        break;
    }
}


GPS::GPS()
{
    /* NMEA parser configuration */
    nmea_parser_config_t config = {
        .uart = {
            .uart_port = UART_NUM_1,
            .rx_pin = BOARD_IO_GPS_TX,
            .baud_rate = 9600,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .event_queue_size = 16
        }
    };
    /* init NMEA parser library */
    ESP_LOGI(TAG, "NMEA parser initializing");
    nmea_parser_handle_t nmea_hdl = nmea_parser_init(&config);
    nmea_parser_add_handler(nmea_hdl, gps_event_handler, NULL);
    globalInstance = this;
}

GPS::~GPS()
{

}