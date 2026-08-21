#include "app_alarm_system.h"

/**
 * @brief 初始化告警系统
 */
void app_alarm_system_init(struct app_alarm_system *cb)
{
    uint8_t i;

    if (cb == NULL)
    {
        return;
    }

    rt_memset(cb, 0, sizeof(*cb));

    /// initial al alarm object
    for (i = 0; i < APP_ALARM_SYSTEM_ID_NUM; i++)
    {
        alarm_init(&cb->alarms[i]);
        cb->alarms[i].alarm_id = i;
    }
}

int8_t app_alarm_system_register(struct app_alarm_system *cb, const struct alarm_config_t *config)
{
    if (cb == NULL || config == NULL)
    {
        return -1;
    }

    if (config->alarm_id >= APP_ALARM_SYSTEM_ID_NUM)
    {
        return -1;
    }

    return alarm_config(&cb->alarms[config->alarm_id], config);
}



#if 1

int8_t app_alarm_system_enable_alarm(struct app_alarm_system *cb, uint8_t alarm_id)
{
    if (cb == NULL || alarm_id >= APP_ALARM_SYSTEM_ID_NUM)
    {
        return -1;
    }

    alarm_enable(&cb->alarms[alarm_id]);

    return 0;
}

int8_t app_alarm_system_disable_alarm(struct app_alarm_system *cb, uint8_t alarm_id)
{
    if (cb == NULL || alarm_id >= APP_ALARM_SYSTEM_ID_NUM)
    {
        return -1;
    }

    alarm_disable(&cb->alarms[alarm_id]);
    return 0;
}

int8_t app_alarm_system_clear_alarm(struct app_alarm_system *cb, uint8_t alarm_id)
{
    if (cb == NULL || alarm_id >= APP_ALARM_SYSTEM_ID_NUM)
    {
        return -1;
    }

    alarm_clear(&cb->alarms[alarm_id]);
    return 0;
}

void app_alarm_system_clear_all(struct app_alarm_system *cb)
{
    uint8_t i;

    if (cb == NULL)
    {
        return;
    }

    for (i = 0; i < APP_ALARM_SYSTEM_ID_NUM; i++)
    {
        alarm_clear(&cb->alarms[i]);
    }

    cb->active_alarm_count = 0;
}

enum alarm_status_t app_alarm_system_get_status(const struct app_alarm_system *cb, uint8_t alarm_id)
{
    if (cb == NULL || alarm_id >= APP_ALARM_SYSTEM_ID_NUM)
    {
        return ALARM_STATUS_DISABLED;
    }

    return  alarm_get_status(&cb->alarms[alarm_id]);
}

uint8_t app_alarm_system_has_active_alarms(const struct app_alarm_system *cb)
{
    if (cb == NULL)
    {
        return 0;
    }

    return (cb->active_alarm_count > 0) ? 1 : 0;
}

uint8_t app_alarm_system_get_active_count(const struct app_alarm_system *cb)
{
    if (cb == NULL)
    {
        return 0;
    }

    return cb->active_alarm_count;
}

struct alarm *app_alarm_system_get_alarm(struct app_alarm_system *cb, uint8_t alarm_id)
{
    if (cb == NULL || alarm_id >= APP_ALARM_SYSTEM_ID_NUM)
    {
        return NULL;
    }

    return &cb->alarms[alarm_id];
}

uint32_t app_alarm_system_get_trigger_count(const struct app_alarm_system *cb, uint8_t alarm_id)
{
    if (cb == NULL || alarm_id >= APP_ALARM_SYSTEM_ID_NUM)
    {
        return 0;
    }

    return 0u;
}

void app_alarm_system_poll(struct app_alarm_system *cb)
{

    uint8_t active_count;
    uint8_t i;

    if (cb == NULL)
    {
        return;
    }

#if 0
    cb->system_run_count++;
#endif

    active_count = 0;

    for (i = 0; i < APP_ALARM_SYSTEM_ID_NUM; i++)
    {
        alarm_run(&cb->alarms[i]);

        /// 统计告警
        if (cb->alarms[i].status == ALARM_STATUS_ACTIVE)
        {
            active_count++;
        }
    }

    cb->active_alarm_count = active_count;

#if 0
    /// 更新总告警次数统计
    cb->total_alarm_count = 0;

    for (i = 0; i < APP_ALARM_SYSTEM_ID_NUM; i++)
    {
        cb->total_alarm_count += cb->alarms[i].trigger_count;
    }
#endif
}

#endif