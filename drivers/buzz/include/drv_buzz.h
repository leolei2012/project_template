#ifndef DRV_BUZZ_H
#define DRV_BUZZ_H

#include "platform.h"



struct drv_buzz
{
    void (*set_buzz)(enum on_off_t onoff);
};

void drv_buzz_init(struct drv_buzz *cb);


#endif // DRV_BUZZ_H