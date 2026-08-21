#ifndef APP_SCREEN_H
#define APP_SCREEN_H

#include "platform.h"

enum app_screen_state_t
{
    APP_SCREEN_STATE_INIT = 0,
    APP_SCREEN_STATE_SCREEN_INIT_OK,
    APP_SCREEN_STATE_SCREEN_RUNNING
};

struct app_screen
{
    enum app_screen_state_t state;






    uint16_t error_code_switch_tick;
    uint8_t error_code_display_index;





};

void app_screen_init(struct app_screen *cb);
void app_screen_trigger_child_lock_flash(struct app_screen *cb);
void app_screen_poll(struct app_screen *cb);



#endif // APP_SCREEN_H