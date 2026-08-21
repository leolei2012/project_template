#include "app.h"
#include "drv.h"

#include "middleware.h"

struct app g_app;

static struct app_core s_app_core;
static struct app_hmi s_app_hmi;
static struct app_alarm_system s_app_alarm_system;
static struct app_water_pump s_app_water_pump;
static struct app_compressor s_app_compressor;
static struct app_fan s_app_fan;
static struct app_memory s_app_memory;

int app_init(void)
{

    /// core --------------------------------------------------------------------
    app_core_init(&s_app_core);
    g_app.core = &s_app_core;

    /// hmi ---------------------------------------------------------------------
    app_hmi_init(&s_app_hmi);
    g_app.hmi = &s_app_hmi;

    /// water pump --------------------------------------------------------------
    app_water_pump_init(&s_app_water_pump);
    g_app.water_pump = &s_app_water_pump;

    /// compressor --------------------------------------------------------------
    app_compressor_init(&s_app_compressor);
    g_app.compressor = &s_app_compressor;

    /// fan ---------------------------------------------------------------------
    app_fan_init(&s_app_fan);
    g_app.fan = &s_app_fan;

#if 1 /// alarm system ----------------------------------------------------------
    app_alarm_system_init(&s_app_alarm_system);
    g_app.alarm_system = &s_app_alarm_system;

#endif // ----------------------------------------------------------------------

    /// memory ------------------------------------------------------------------
    app_memory_init(&s_app_memory);
    g_app.memory = &s_app_memory;

    return 0;
}
INIT_ENV_EXPORT(app_init);

#if 1 /// core

static rt_thread_t app_core_thread_ptr;

void app_core_task_entry(void *parameter)
{
    while (1)
    {
        app_core_poll(g_app.core);
        app_fan_poll(g_app.fan);
        app_water_pump_poll(g_app.water_pump);
        app_compressor_poll(g_app.compressor);

        rt_thread_mdelay(APP_CORE_TASK_PERIOD);
    }
}

int app_core_task_init(void)
{

    app_core_thread_ptr = rt_thread_create("core",
                                           app_core_task_entry,
                                           NULL,
                                           APP_CORE_TASK_STACK_SIZE,
                                           10,
                                           25);

    if (app_core_thread_ptr != RT_NULL)
        rt_thread_startup(app_core_thread_ptr);

    return 0;
}
INIT_APP_EXPORT(app_core_task_init);

#endif

#if 1 /// hmi

static rt_thread_t app_hmi_thread_ptr;

void app_hmi_task_entry(void *parameter)
{
    while (1)
    {
        app_hmi_poll(g_app.hmi);

        rt_thread_mdelay(APP_HMI_TASK_PERIOD);
    }
}

int app_hmi_task_init(void)
{

    app_hmi_thread_ptr = rt_thread_create("hmi",
                                          app_hmi_task_entry,
                                          NULL,
                                          APP_HMI_TASK_STACK_SIZE,
                                          10,
                                          25);

    if (app_hmi_thread_ptr != RT_NULL)
        rt_thread_startup(app_hmi_thread_ptr);

    return 0;
}
INIT_APP_EXPORT(app_hmi_task_init);

#endif

#if 1 /// alarm_system

static rt_thread_t app_alarm_system_thread_ptr;

void app_alarm_system_task_entry(void *parameter)
{
    while (1)
    {
        app_alarm_system_poll(g_app.alarm_system);

        rt_thread_mdelay(APP_ALARM_SYSTEM_TASK_PERIOD);
    }
}

int app_alarm_system_task_init(void)
{
    struct alarm_config_t alarm_config;

#if 0  /**< WiFi module not yet implemented */
    alarm_config.alarm_id = APP_ALARM_SYSTEM_ID_WIFI_DISCONNECTED;
    alarm_config.threshold_type = EQUAL_TO_THRESHOLD;
    alarm_config.threshold_value = 0;                                    /// 阈值 0
    alarm_config.recovery_diff_value = 0;                                /// 回差值 0
    alarm_config.data_source = &g_middleware.monitor->wifi.is_connected; /// 数据源
    alarm_config.duration = 0;                                           /// 0秒触发
    alarm_config.recovery_time = 0;                                      /// 0秒恢复
    alarm_config.callback = NULL;                                        /// 回调函数
    app_alarm_system_register(g_app.alarm_system, &alarm_config);
#endif

#if 1
    alarm_config.alarm_id = APP_ALARM_SYSTEM_ID_EVA_TEMP_SENSOR_FAILURE;
    alarm_config.threshold_type = EQUAL_TO_THRESHOLD;
    alarm_config.threshold_value = 1;                                    /// 阈值 0
    alarm_config.recovery_diff_value = 0;                                /// 回差值 0
    alarm_config.data_source = &g_drv.ain_sensor->eva_temp_sensor.error; /// 数据源
    alarm_config.duration = 30;                                          /// 0秒触发
    alarm_config.recovery_time = 30;                                     /// 0秒恢复
    alarm_config.callback = NULL;                                        /// 回调函数
    app_alarm_system_register(g_app.alarm_system, &alarm_config);
#endif

#if 1
    alarm_config.alarm_id = APP_ALARM_SYSTEM_ID_HUMIDITY_SENSOR_FAILURE;
    alarm_config.threshold_type = EQUAL_TO_THRESHOLD;
    alarm_config.threshold_value = 1;                                    /// 阈值 0
    alarm_config.recovery_diff_value = 0;                                /// 回差值 0
    alarm_config.data_source = &g_drv.ain_sensor->humidity_sensor.error; /// 数据源
    alarm_config.duration = 30;                                          /// 0秒触发
    alarm_config.recovery_time = 30;                                     /// 0秒恢复
    alarm_config.callback = NULL;                                        /// 回调函数
    app_alarm_system_register(g_app.alarm_system, &alarm_config);
#endif

#if 1
    alarm_config.alarm_id = APP_ALARM_SYSTEM_ID_AMBIENT_TEMP_SENSOR_FAILURE;
    alarm_config.threshold_type = EQUAL_TO_THRESHOLD;
    alarm_config.threshold_value = 1;                                        /// 阈值 0
    alarm_config.recovery_diff_value = 0;                                    /// 回差值 0
    alarm_config.data_source = &g_drv.ain_sensor->ambient_temp_sensor.error; /// 数据源
    alarm_config.duration = 30;                                              /// 0秒触发
    alarm_config.recovery_time = 30;                                         /// 0秒恢复
    alarm_config.callback = NULL;                                            /// 回调函数
    app_alarm_system_register(g_app.alarm_system, &alarm_config);
#endif

#if 1
    alarm_config.alarm_id = APP_ALARM_SYSTEM_ID_INVERTER_DISCONNECTED;
    alarm_config.threshold_type = EQUAL_TO_THRESHOLD;
    alarm_config.threshold_value = 1;                  /// 阈值 0
    alarm_config.recovery_diff_value = 0;              /// 回差值 0
    alarm_config.data_source = &g_drv.inverter->error; /// 数据源
    alarm_config.duration = 30;                        /// 0秒触发
    alarm_config.recovery_time = 30;                   /// 0秒恢复
    alarm_config.callback = NULL;                      /// 回调函数
    app_alarm_system_register(g_app.alarm_system, &alarm_config);
#endif


#if 1
    if (g_app.memory != NULL)
    {
        alarm_config.alarm_id = APP_ALARM_SYSTEM_ID_MEMORY_FAILURE;
        alarm_config.threshold_type = EQUAL_TO_THRESHOLD;
        alarm_config.threshold_value = 1;
        alarm_config.recovery_diff_value = 0;
        alarm_config.data_source = &g_app.memory->storage_fault;
        alarm_config.duration = 0;
        alarm_config.recovery_time = 0;
        alarm_config.callback = NULL;
        app_alarm_system_register(g_app.alarm_system, &alarm_config);
    }
#endif

    app_alarm_system_thread_ptr = rt_thread_create("alarm",
                                                   app_alarm_system_task_entry,
                                                   NULL,
                                                   APP_ALARM_SYSTEM_TASK_STACK_SIZE,
                                                   10,
                                                   25);

    if (app_alarm_system_thread_ptr != RT_NULL)
        rt_thread_startup(app_alarm_system_thread_ptr);

    return 0;
}
INIT_APP_EXPORT(app_alarm_system_task_init);

#endif

#if 1

rt_thread_t app_memory_thread_ptr;

void app_memory_task_entry(void *parameter)
{
    while (1)
    {
        app_memory_poll(g_app.memory);

        rt_thread_mdelay(APP_MEMORY_TASK_PERIOD);
    }
}

int app_memory_task_init(void)
{

    app_memory_thread_ptr = rt_thread_create("memory",
                                             app_memory_task_entry,
                                             NULL,
                                             APP_MEMORY_TASK_STACK_SIZE,
                                             10,
                                             25);

    if (app_memory_thread_ptr != RT_NULL)
        rt_thread_startup(app_memory_thread_ptr);

    return 0;
}
INIT_APP_EXPORT(app_memory_task_init);

#endif