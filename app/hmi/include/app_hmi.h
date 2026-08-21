#ifndef APP_HMI_H
#define APP_HMI_H

#include "platform.h"

#include "app_buzz.h"
#include "app_key.h"
#include "app_led.h"
#include "app_screen.h"

enum app_hmi_state_t
{
    APP_HMI_STATE_INIT,
    APP_HMI_STATE_INIT_OK,
    APP_HMI_STATE_RUNNING

};

struct app_hmi
{
    enum app_hmi_state_t state; /// HMI

    uint8_t tick;

    struct app_buzz buzz;
    struct app_key key;
    struct app_led led;
    struct app_screen screen;


};

void app_hmi_init(struct app_hmi *cb);

#define APP_HMI_TASK_PERIOD 10
void app_hmi_poll(struct app_hmi *cb);

#endif // APP_HMI_H