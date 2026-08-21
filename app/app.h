#ifndef APP_H
#define APP_H

#include "platform.h"

#include "app_alarm_system.h"
#include "app_core.h"
#include "app_hmi.h"
#include "app_water_pump.h"
#include "app_fan.h"
#include "app_compressor.h"
#include "app_memory.h"


#define APP_ALARM_SYSTEM_TASK_STACK_SIZE 512u

#define APP_CORE_TASK_STACK_SIZE 512u
#define APP_HMI_TASK_STACK_SIZE 512u
#define APP_MEMORY_TASK_STACK_SIZE 512u

extern struct app g_app;

struct app
{
    struct app_core *core;

    struct app_water_pump *water_pump;
    struct app_fan *fan;
    struct app_compressor *compressor;

    struct app_hmi *hmi;
    struct app_alarm_system *alarm_system;
    struct app_memory *memory;
};

#endif // APP_H