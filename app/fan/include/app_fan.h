#ifndef APP_FAN_H
#define APP_FAN_H

#include "platform.h"
#include "app.h"
#define APP_FAN_STOP_DELAY_TICKS (100u)

#define APP_CORE_FAN_SPEED_LOW_RPM 1000u
#define APP_CORE_FAN_SPEED_MID_RPM 1200u
#define APP_CORE_FAN_SPEED_HIGH_RPM 1500u



struct app_fan
{


    uint16_t fan_stop_delay_tick;
    uint16_t run_tick;
};

void app_fan_init(struct app_fan *cb);

#define APP_FAN_TASK_PERIOD 100
void app_fan_poll(struct app_fan *cb);




#endif // APP_FAN_H