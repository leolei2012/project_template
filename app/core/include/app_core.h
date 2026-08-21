#ifndef APP_CORE_H
#define APP_CORE_H

#include "platform.h"

enum app_core_mode_t
{
    APP_CORE_MODE_NONE = 0,
    APP_CORE_MODE_AUTO,
    APP_CORE_MODE_SLEEP,
    APP_CORE_MODE_TURBO,
    APP_CORE_MODE_TEST

};

enum app_core_state_t
{
    APP_CORE_STATE_INIT = 0,
    APP_CORE_STATE_INIT_OK,
    APP_CORE_STATE_RUNNING,
    APP_CORE_STATE_ERROR
};

enum app_core_timer_state_t
{
    APP_CORE_TIMER_STATE_NO_TIMER = 0,
    APP_CORE_TIMER_STATE_READY,
    APP_CORE_TIMER_STATE_STOP,
    APP_CORE_TIMER_STATE_START,
};

enum app_core_fan_speed_t
{
    APP_CORE_FAN_SPEED_LOW = 0,
    APP_CORE_FAN_SPEED_MID,
    APP_CORE_FAN_SPEED_HIGH
};

struct app_core
{

    enum app_core_state_t state;

    struct
    {
        uint16_t power_onoff;
        uint16_t power_onoff_cache;
        uint16_t switch_tick;
        uint8_t is_editing;
    } power;

    struct
    {
        uint16_t uv_light_onoff;
        uint16_t uv_light_onoff_cache;
        uint16_t switch_tick;
        uint8_t is_editing;
    } uv_light;

    struct
    {
        uint16_t anion_light_onoff;
        uint16_t anion_light_onoff_cache;
        uint16_t switch_tick;
        uint8_t is_editing;
    } anion_light;

    struct
    {
        uint16_t set_humidity; /// (0-100)%
        uint16_t set_humidity_cache;
        uint16_t switch_tick;
        uint8_t is_editing;

    } humidity;

    struct
    {
        uint16_t mode;       /// enum app_core_mode_t
        uint16_t mode_cache; /// enum app_core_mode_t
        uint16_t switch_tick;
        uint8_t is_editing;

    } work_mode;

    struct
    {
        uint16_t set_fan_speed;       /// app_fan_speed_t
        uint16_t set_fan_speed_cache; /// app_fan_speed_t
        uint16_t switch_tick;
        uint8_t is_editing;

    } fan_speed;

    uint16_t child_lock;

    struct
    {
        uint8_t ready_tick;
        uint16_t start_tick;

        uint16_t state;   ///  app_core_timer_state_t
        uint16_t hours;   /// (0-24)h
        uint16_t minutes; /// (0-59)m

    } timer;

    struct
    {
        uint16_t hour;   /// (0-24)h
        uint16_t minute; /// (0-59)m
    } clock;
};

void app_core_init(struct app_core *cb);

#define APP_CORE_TASK_PERIOD 100
void app_core_poll(struct app_core *cb);

#endif // APP_CORE_H