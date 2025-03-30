#pragma once

#include "nmea_parser.h"

#ifdef __cplusplus

#include "observer.hpp"

static void gps_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

class GPS : public Subject<gps_t> {
public:
    GPS();
    ~GPS();

    void init();
    const gps_t& getData() const
    {
        return data;
    }

    friend void gps_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

private:
    gps_t data;
    nmea_parser_handle_t nmea_hdl; // NMEA parser handle
};


#endif

