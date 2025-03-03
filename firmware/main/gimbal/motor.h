/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once
#include <stdint.h>


class Motor {
public:
    Motor(uint8_t id);
    
   void set_pwm(int32_t percent);

private:
    uint8_t mot_id;
    static bool is_init;
};

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif
