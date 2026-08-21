#ifndef DRV_H
#define DRV_H

extern struct drv g_drv;

#include "platform.h"

///#include "drv_buzz.h"
///#include "output/drv_output.h"
///#include "drv_memory.h"

#include "drv_ain_sensor.h"
#include "drv_curr_fdbk.h"
#include "uart_control.h"




#define DRV_INVERTER_TASK_STACK_SIZE 512u
#define DRV_AIN_SENSOR_TASK_STACK_SIZE 1024u
#define DRV_CURR_FDBK_TASK_STACK_SIZE 1024u



struct drv
{
    struct drv_ain_sensor *ain_sensor;

    struct drv_curr_fdbk *curr_fdbk;

    struct uart_control *uart;

};

int drv_init(void);


#endif // DRV_H