#ifndef APP_WATER_PUMP_H
#define APP_WATER_PUMP_H

#include "platform.h"
#include "app.h"


enum
{
    APP_CORE_WATER_FULL_ACTIVE_TICKS = (3000u / APP_CORE_TASK_PERIOD),
    APP_CORE_WATER_FULL_INACTIVE_TICKS = (1000u / APP_CORE_TASK_PERIOD),

    APP_CORE_WATER_PUMP_START_DELAY_TICKS = (150u),
    APP_CORE_WATER_PUMP_RUN_TICKS = (300u),
    APP_CORE_WATER_PUMP_MAX_CYCLES = 3u,

};

enum app_core_water_pump_state_t
{
    APP_WATER_PUMP_STATE_IDLE = 0,
    APP_WATER_PUMP_STATE_WAIT_START,
    APP_WATER_PUMP_STATE_DRAINING,
    APP_WATER_PUMP_STATE_FAULT,
};

struct app_water_pump
{
    uint16_t water_pump_mode;
    uint16_t water_full_status; /// water full flag
    uint16_t water_pump_fault_status;

    uint16_t water_full_active_ticks;
    uint16_t water_full_inactive_ticks;

    uint16_t water_pump_state; /// enum app_core_water_pump_state_t
    uint16_t water_pump_start_delay_tick;
    uint16_t water_pump_run_tick;
    uint8_t water_pump_cycle_count;
};

void app_water_pump_init(struct app_water_pump *cb);

#define APP_WATER_PUMP_TASK_PERIOD 100
void app_water_pump_poll(struct app_water_pump *cb);

#endif // APP_WATER_PUMP_H