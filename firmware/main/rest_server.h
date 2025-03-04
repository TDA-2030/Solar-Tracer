/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once

#include <iostream>
#include "gimbal.h"
#include "observer.hpp"
#include "esp_http_server.h"

class SensorWeb : public Observer<imu_data_t> {
public:
    void update(const imu_data_t &data) override
    {
        imu_data = data;
    }

    const imu_data_t& getIMUData() const {
        return imu_data;
    }

private:
    /* data */
    imu_data_t imu_data;
};

typedef esp_err_t (*httpd_uri_handler_t)(httpd_req_t *r);

class WebServer {
public:
    WebServer(const char *base_path);
    ~WebServer();
    esp_err_t start();
    esp_err_t stop();
    esp_err_t on(const char*url, httpd_method_t method, httpd_uri_handler_t handler, void* user_ctx);

private:
    /* data */
    const char *base_path;
    httpd_handle_t server = NULL;

    std::shared_ptr<SensorWeb> sensorWeb;
};

extern Gimbal gimbal;


#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
