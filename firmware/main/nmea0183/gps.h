#pragma once

#include "nmea_parser.h"

#ifdef __cplusplus

#include "observer.hpp"

class GPS : public Subject<gps_t> {
public:
    GPS();
    ~GPS();

};


#endif

