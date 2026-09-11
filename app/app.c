#include "app.h"

struct app g_app;

static struct app_alarm_system s_app_alarm_system;

/**
 * 应用层组合根：实例化 + 注入（零业务逻辑）。
 * 告警数据源来自 drivers 层（如 g_drv.ain_sensor 的某个物理量），
 * 在 app_alarm_system_task_init 里用 app_alarm_system_register 注册。
 */
int app_init(void)
{
    app_alarm_system_init(&s_app_alarm_system);
    g_app.alarm_system = &s_app_alarm_system;

    return 0;
}
INIT_ENV_EXPORT(app_init);

static rt_thread_t app_alarm_system_thread_ptr;

static void app_alarm_system_task_entry(void *parameter)
{
    (void)parameter;

    while (1)
    {
        app_alarm_system_poll(g_app.alarm_system);

        rt_thread_mdelay(APP_ALARM_SYSTEM_TASK_PERIOD);
    }
}

int app_alarm_system_task_init(void)
{
    app_alarm_system_thread_ptr = rt_thread_create("alarm",
                                                   app_alarm_system_task_entry,
                                                   RT_NULL,
                                                   APP_ALARM_SYSTEM_TASK_STACK_SIZE,
                                                   10,
                                                   25);

    if (app_alarm_system_thread_ptr != RT_NULL)
    {
        rt_thread_startup(app_alarm_system_thread_ptr);
    }

    return 0;
}
INIT_APP_EXPORT(app_alarm_system_task_init);
