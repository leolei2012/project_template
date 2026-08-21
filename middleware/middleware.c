#include "middleware.h"

#include "drv.h"

#include "bl.h"
#include "bl_port_stm32g4.h"

#include <rtthread.h>

struct middleware g_middleware;

static debug_monitor s_debug_monitor;

int middleware_init(void)
{
    /** OTA：绑定 flash 端口 + 初始化下载引擎（APP 侧只下载到 APP2 + 写 IAP_REQUEST，copy 交给 bootloader） */
    bl_port_stm32g4_init();
    bl_init(bl_port_stm32g4_get());

    /** debug_monitor 自行绑定 uart_control 传输 + dm_adapter 寄存器/适配器（uart 实例在此注入） */
    debug_monitor_init(&s_debug_monitor, 0x01, g_drv.uart);

    g_middleware.debug_monitor = &s_debug_monitor;

    return 0;
}
INIT_ENV_EXPORT(middleware_init);

/** middleware 通信线程：周期驱动 debug_monitor（含 APP 侧 OTA commit 写 IAP_REQUEST + reset） */
static rt_thread_t middleware_thread_ptr;

static void middleware_task_entry(void *parameter)
{
    (void)parameter;

    while (1)
    {
        debug_monitor_poll(&s_debug_monitor);
        rt_thread_mdelay(10);
    }
}

static int middleware_task_init(void)
{
    middleware_thread_ptr = rt_thread_create("middleware",
                                             middleware_task_entry,
                                             NULL,
                                             2048,
                                             15,
                                             25);

    if (middleware_thread_ptr != RT_NULL)
    {
        rt_thread_startup(middleware_thread_ptr);
    }

    return 0;
}
INIT_ENV_EXPORT(middleware_task_init);
