#include "drv_buzz.h"

#include "hal.h"







void drv_buzz_init(struct drv_buzz *cb)
{
    if (cb == NULL)
    {
        return;
    }

    memset(cb, 0, sizeof(*cb));


}
















