
#include "nmea_parser.h"
#include "esp_log.h"
#include "gps.h"
#include "board.h"

static const char *TAG = "GPS";


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

    GPS *pgps = (GPS *)event_handler_arg;
    switch (event_id) {
    case GPS_UPDATE: {
        gps_t *gps = (gps_t *)event_data;
        pgps->data = *gps;
        /* print information parsed from GPS statements */
        if (gps->valid) {
            ESP_LOGI(TAG, "latitude:%.3f, longitude:%.3f, altitude:%.3f, speed:%.1f, sats_in_view:%d, UTC time:%d-%d-%d %d:%d:%d",
                     gps->latitude, gps->longitude, gps->altitude, gps->speed, gps->sats_in_view, gps->date.year + YEAR_BASE, gps->date.month, gps->date.day,
                     gps->tim.hour + TIME_ZONE, gps->tim.minute, gps->tim.second);
            pgps->notifyObservers(*gps);
        } else {
            ESP_LOGW(TAG, "invalid GPS data");
        }

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
    nmea_hdl = nullptr;
    data.valid = false;
    data.longitude = 112.933333;
    data.latitude = 28.183333;
}

GPS::~GPS()
{

}

void GPS::init()
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
    nmea_hdl = nmea_parser_init(&config);
    nmea_parser_add_handler(nmea_hdl, gps_event_handler, this);
}
