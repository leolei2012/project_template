#ifndef APP_FREQUENCY_H
#define APP_FREQUENCY_H

#include "platform.h"

#define APP_FREQUENCY_PLATFORM_50HZ_X10 APP_FREQUENCY_F2_X10

#define APP_FREQUENCY_F1_X10 400
#define APP_FREQUENCY_F2_X10 500
#define APP_FREQUENCY_F3_X10 580
#define APP_FREQUENCY_F4_X10 650
#define APP_FREQUENCY_F5_X10 750

#define APP_FREQUENCY_MIN_X10 300u
#define APP_FREQUENCY_MAX_X10 650u
#define APP_FREQUENCY_LIMIT_X10 500u
#define APP_FREQUENCY_PROTECTED_X10 400u

struct app_frequency
{
    uint16_t target_frequency; /// X0.1 Hz
    uint16_t frequency;        /// X0.1 Hz
};

void app_frequency_init(struct app_frequency *cb);

void app_frequency_set_target_frequency(struct app_frequency *cb, uint16_t target_frequency);

uint16_t app_frequency_apply_fan_limit(struct app_frequency *cb, uint16_t frequency);

uint16_t app_frequency_get_startup_frequency_x10(struct app_frequency *cb);
uint16_t app_frequency_get_target_frequency_x10(struct app_frequency *cb);

#endif // APP_FREQUENCY_H