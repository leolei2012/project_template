#ifndef APP_COMPRESSOR_H
#define APP_COMPRESSOR_H

#include "platform.h"
#include "app_frequency.h"

#define APP_CORE_COMPRESSOR_ON_HYSTERESIS_RH 3u
#define APP_CORE_COMPRESSOR_OFF_HYSTERESIS_RH 2u

#define APP_CORE_COMPRESSOR_STARTUP_PROTECTED_40HZ_X10 400u
#define APP_CORE_COMPRESSOR_STOP_PROTECT_TICKS (1800u)
#define APP_CORE_COMPRESSOR_START_PLATFORM_TICKS (600u)
#define APP_CORE_COMPRESSOR_STARTUP_TOTAL_TICKS (1800u)
#define APP_CORE_COMPRESSOR_MINIMUM_RUNNING_TICKS (1800u)
#define APP_CORE_COMPRESSOR_REST_STOP_TICKS (288000u) /// run 8 hour, rest 8 minute
#define APP_CORE_COMPRESSOR_REST_TICKS (4800u)        /// run 8 hour, rest 8 minute
#define APP_CORE_COMPRESSOR_STOP_PLATFORM_TICKS (100u)


enum app_compressor_protection_status_t
{
    APP_COMPRESSOR_PROTECTION_STATUS_NOMAL = 0,
    APP_COMPRESSOR_PROTECTION_STATUS_SLOW_RISE,
    APP_COMPRESSOR_PROTECTION_STATUS_LIMIT,
    APP_COMPRESSOR_PROTECTION_STATUS_SLOW_FALL,
    APP_COMPRESSOR_PROTECTION_STATUS_FAST_FALL,
    APP_COMPRESSOR_PROTECTION_STATUS_ABNORMAL
};


enum app_core_compressor_state_t
{
    APP_COMPRESSOR_STATE_STOP = 0,
    APP_COMPRESSOR_STATE_STARTUP_3MIN,
    APP_COMPRESSOR_STATE_RUNNING,
    APP_COMPRESSOR_STATE_STOPPING_PLATFORM,
};





struct app_compressor
{
    uint16_t compressor_onoff;
    uint8_t compressor_rest_flag;
    uint16_t compressor_state; /// enum app_core_compressor_state_t
    uint32_t compressor_state_tick;
    uint16_t compressor_startup_frequency;

    struct app_frequency compressor_frequency;

    struct
    {
        uint16_t v_dc_protection; /// enum app_compressor_protection_status_t
        uint16_t v_dc_protection_abnormal_tick;
        uint16_t v_dc_protection_recover_tick;
        uint16_t v_ac_protection;
        uint16_t i_ac_protection;
        uint16_t i_compressor_rms_protection;
        uint16_t freeze_protection;
        uint16_t exhaust_high_temp_protection;
    } protection;
};


void app_compressor_init(struct app_compressor *cb);

#define APP_COMPRESSOR_TASK_PERIOD 100
void app_compressor_poll(struct app_compressor *cb);

#endif // APP_COMPRESSOR_H