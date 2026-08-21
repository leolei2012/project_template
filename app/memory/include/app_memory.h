#ifndef APP_MEMORY_H
#define APP_MEMORY_H

#include "platform.h"

struct app_memory_settings_blob_t
{
    uint16_t version;
    uint16_t size;
    uint16_t power_onoff;
    uint16_t uv_light_onoff;
    uint16_t anion_light_onoff;
    uint16_t set_humidity;
    uint16_t work_mode;
    uint16_t fan_speed;
    uint16_t child_lock;
    uint16_t water_pump_mode;
};

struct app_memory
{
    struct app_memory_settings_blob_t saved_settings;
    struct app_memory_settings_blob_t pending_settings;
    uint16_t stable_ticks;
    uint16_t storage_fault;
    uint8_t has_saved_settings;
    uint8_t dirty;
};

#define APP_MEMORY_TASK_PERIOD 1000u

void app_memory_init(struct app_memory *cb);
void app_memory_poll(struct app_memory *cb);

#endif // APP_MEMORY_H