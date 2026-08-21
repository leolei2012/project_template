#include "app_core.h"

#include "middleware.h"
#include "app.h"
#include "drv.h"

enum
{
    APP_CORE_TIMER_READY_TICKS = (3000u + APP_CORE_TASK_PERIOD - 1u) / APP_CORE_TASK_PERIOD,
    APP_CORE_TIMER_MINUTE_TICKS = (60000u + APP_CORE_TASK_PERIOD - 1u) / APP_CORE_TASK_PERIOD,
    APP_CORE_DATA_SWITCH_TICKS = (600u + APP_CORE_TASK_PERIOD - 1u) / APP_CORE_TASK_PERIOD,
};

static void app_core_timer_logic(struct app_core *cb)
{
    if (cb == NULL)
    {
        return;
    }

    if (cb->power.power_onoff == OFF)
    {
        cb->timer.state = APP_CORE_TIMER_STATE_NO_TIMER;
    }

    switch (cb->timer.state)
    {
    case APP_CORE_TIMER_STATE_NO_TIMER:
        cb->timer.minutes = 0;
        cb->timer.hours = 0;
        cb->timer.ready_tick = 0;
        break;

    case APP_CORE_TIMER_STATE_READY:
        cb->timer.ready_tick++;
        if (cb->timer.ready_tick >= APP_CORE_TIMER_READY_TICKS)
        {
            cb->timer.ready_tick = 0;
            cb->timer.state = APP_CORE_TIMER_STATE_START;
            cb->timer.start_tick = 0;
        }
        break;

    case APP_CORE_TIMER_STATE_STOP:

        cb->power.power_onoff = OFF;
        cb->uv_light.uv_light_onoff = OFF;
        cb->timer.hours = 0u;
        cb->timer.minutes = 0u;
        cb->timer.state = APP_CORE_TIMER_STATE_NO_TIMER;
        break;

    case APP_CORE_TIMER_STATE_START:
        if (cb->timer.minutes > 0u)
        {
            cb->timer.start_tick++;
            if (cb->timer.start_tick >= APP_CORE_TIMER_MINUTE_TICKS)
            {
                cb->timer.start_tick = 0;
                cb->timer.minutes--;
            }
        }
        else if (cb->timer.hours > 0u)
        {
            cb->timer.hours--;
            cb->timer.minutes = 59u;
        }
        else
        {
            cb->timer.state = APP_CORE_TIMER_STATE_STOP;
        }
        break;

    default:
        break;
    }
}

/*
 * Applies the pending cache value to the live field after a 3-second hold.
 * Returns 1 if the value was committed this tick, 0 otherwise.
 */
uint8_t app_core_data_switch(uint16_t *data,
                             uint16_t *data_cache,
                             uint8_t *is_editing,
                             uint16_t *switch_tick,
                             uint16_t tick)
{

    if (is_editing == NULL || data == NULL || data_cache == NULL)
        return 0;

    rt_enter_critical();
    if (*is_editing == 0u)
    {

        *data_cache = *data;
        rt_exit_critical();
        return 0u;
    }
    else
    {
        rt_exit_critical();
    }

    (*switch_tick)++;

    if (*switch_tick >= tick)
    {
        *data = *data_cache;
        *is_editing = 0u;
        *switch_tick = 0u;
        return 1u;  /**< committed */
    }

    return 0u;
}

void app_core_set_fan_logic(struct app_core *cb)
{
    int16_t humidity_delta;

    if (cb == NULL)
    {
        return;
    }

    if (g_drv.inverter == NULL)
    {
        return;
    }

    humidity_delta = g_drv.ain_sensor->humidity_sensor.humidity - g_app.core->humidity.set_humidity;

    if (g_app.core->work_mode.mode == APP_CORE_MODE_AUTO)
    {

        if (humidity_delta > 20 && humidity_delta <= 30)
        {
            g_app.core->fan_speed.set_fan_speed = APP_CORE_FAN_SPEED_HIGH;
            g_app.core->fan_speed.set_fan_speed_cache = APP_CORE_FAN_SPEED_HIGH;
        }
        else if (humidity_delta > 10 && humidity_delta <= 20)
        {
            g_app.core->fan_speed.set_fan_speed = APP_CORE_FAN_SPEED_MID;
            g_app.core->fan_speed.set_fan_speed_cache = APP_CORE_FAN_SPEED_MID;
        }
        else if (humidity_delta <= 10)
        {
            g_app.core->fan_speed.set_fan_speed = APP_CORE_FAN_SPEED_LOW;
            g_app.core->fan_speed.set_fan_speed_cache = APP_CORE_FAN_SPEED_LOW;
        }
    }
    else if (g_app.core->work_mode.mode == APP_CORE_MODE_SLEEP)
    {
        g_app.core->fan_speed.set_fan_speed = APP_CORE_FAN_SPEED_LOW;
        g_app.core->fan_speed.set_fan_speed_cache = APP_CORE_FAN_SPEED_LOW;
    }
    else if (g_app.core->work_mode.mode == APP_CORE_MODE_TURBO)
    {
        g_app.core->fan_speed.set_fan_speed = APP_CORE_FAN_SPEED_HIGH;
        g_app.core->fan_speed.set_fan_speed_cache = APP_CORE_FAN_SPEED_HIGH;
    }
    else if (g_app.core->work_mode.mode == APP_CORE_MODE_TEST)
    {
        g_app.core->fan_speed.set_fan_speed = APP_CORE_FAN_SPEED_HIGH;
        g_app.core->fan_speed.set_fan_speed_cache = APP_CORE_FAN_SPEED_HIGH;
    }
}

void app_core_uv_light_logic(struct app_core *cb)
{
    if (cb == NULL)
    {
        return;
    }

    if (cb->power.power_onoff == OFF)
    {
        g_drv.output->uv_lamp_off();
        return;
    }


    if (cb->uv_light.uv_light_onoff == ON)
    {
        g_drv.output->uv_lamp_on();
    }
    else
    {
        g_drv.output->uv_lamp_off();
    }


}

void app_core_anion_light_logic(struct app_core *cb)
{
    if (cb == NULL)
    {
        return;
    }

    if (cb == NULL)
    {
        return;
    }

    if (cb->power.power_onoff == OFF)
    {
        g_drv.output->anion_output_off();
        return;
    }


    if (cb->anion_light.anion_light_onoff == ON)
    {
        g_drv.output->anion_output_on();
    }
    else
    {
        g_drv.output->anion_output_off();
    }
}



void app_core_set_humidity_logic(struct app_core *cb)
{

    if (cb == NULL)
    {
        return;
    }

    if (cb->work_mode.mode == APP_CORE_MODE_TURBO)
    {

        cb->humidity.set_humidity_cache = 35;
    }


}



static void app_core_running_logic(struct app_core *cb)
{

    if (cb == NULL)
    {
        return;
    }

    app_core_timer_logic(cb);
    app_core_set_fan_logic(cb);

    app_core_uv_light_logic(cb);
    app_core_anion_light_logic(cb);
    ///app_core_set_humidity_logic(cb);


    app_core_data_switch(&cb->power.power_onoff,
                         &cb->power.power_onoff_cache,
                         &cb->power.is_editing,
                         &cb->power.switch_tick,
                         APP_CORE_DATA_SWITCH_TICKS);

    app_core_data_switch(&cb->uv_light.uv_light_onoff,
                         &cb->uv_light.uv_light_onoff_cache,
                         &cb->uv_light.is_editing,
                         &cb->uv_light.switch_tick,
                         APP_CORE_DATA_SWITCH_TICKS);

    app_core_data_switch(&cb->anion_light.anion_light_onoff,
                         &cb->anion_light.anion_light_onoff_cache,
                         &cb->anion_light.is_editing,
                         &cb->anion_light.switch_tick,
                         APP_CORE_DATA_SWITCH_TICKS);

    app_core_data_switch(&cb->humidity.set_humidity,
                         &cb->humidity.set_humidity_cache,
                         &cb->humidity.is_editing,
                         &cb->humidity.switch_tick,
                         30);

    app_core_data_switch(&cb->work_mode.mode,
                         &cb->work_mode.mode_cache,
                         &cb->work_mode.is_editing,
                         &cb->work_mode.switch_tick,
                         APP_CORE_DATA_SWITCH_TICKS);

    app_core_data_switch(&cb->fan_speed.set_fan_speed,
                         &cb->fan_speed.set_fan_speed_cache,
                         &cb->fan_speed.is_editing,
                         &cb->fan_speed.switch_tick,
                         APP_CORE_DATA_SWITCH_TICKS);
}

void app_core_init(struct app_core *cb)
{
    if (cb == NULL)
    {
        return;
    }

    memset(cb, 0, sizeof(*cb));
    cb->state = APP_CORE_STATE_INIT;


}

void app_core_poll(struct app_core *cb)
{
    if (cb == NULL)
    {
        return;
    }

    switch (cb->state)
    {
    case APP_CORE_STATE_INIT:
        cb->state = APP_CORE_STATE_INIT_OK;
        break;

    case APP_CORE_STATE_INIT_OK:
        cb->state = APP_CORE_STATE_RUNNING;
        break;

    case APP_CORE_STATE_RUNNING:
        app_core_running_logic(cb);
        break;

    default:
        break;
    }
}