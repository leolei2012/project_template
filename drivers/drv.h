#ifndef DRV_H
#define DRV_H

extern struct drv g_drv;

#include "platform.h"

#include "drv_ain_sensor.h"
#include "uart_control.h"

#define DRV_AIN_SENSOR_TASK_STACK_SIZE 1024u

struct drv
{
    struct drv_ain_sensor *ain_sensor;

    struct uart_control *uart;
};

int drv_init(void);

#endif // DRV_H
