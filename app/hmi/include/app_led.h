#ifndef APP_LED_H
#define APP_LED_H

#include "platform.h"





struct app_led
{
    uint16_t tick;

};


void app_led_init(struct app_led *cb);

void app_led_poll(struct app_led *cb);



#endif // APP_LED_H