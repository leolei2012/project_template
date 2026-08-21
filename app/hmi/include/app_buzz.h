#ifndef APP_BUZZ_H
#define APP_BUZZ_H

#include "platform.h"

enum app_buzz_state_t
{
    APP_BUZZ_SILENT = 0,
    APP_BUZZ_BEEP,
    APP_BUZZ_BEEP_DURATION,
    APP_BUZZ_BEEP_SOMETIMES,
};

struct app_buzz
{
    enum app_buzz_state_t state;
    uint16_t tick;

    uint16_t beep_tick_reload;
    uint16_t silent_tick_reload;
    uint16_t beep_count;
};

void app_buzz_set_state(struct app_buzz *cb, enum app_buzz_state_t state, uint16_t beep_tick, uint16_t silent_tick, uint16_t beep_count);
void app_buzz_init(struct app_buzz *cb);
void app_buzz_poll(struct app_buzz *cb);

#endif // APP_BUZZ_H