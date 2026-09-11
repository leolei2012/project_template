#ifndef APP_H
#define APP_H

#include "platform.h"

#include "app_alarm_system.h"

#define APP_ALARM_SYSTEM_TASK_STACK_SIZE 512u

extern struct app g_app;

struct app
{
    struct app_alarm_system *alarm_system;
};

#endif // APP_H
