#include "drv.h"

#include "hal.h"
#include "hal_tim1.h"

struct drv g_drv;

static struct drv_ain_sensor s_ain_sensor;
static struct drv_curr_fdbk s_curr_fdbk;

#define UART_RX_BUF_SIZE 256u
static uint8_t s_uart_rx_buf[UART_RX_BUF_SIZE];
static struct uart_control s_uart;

static const struct uart_control_hal_ops s_uart_hal_ops =
    {
        .send = hal_usart2_dma_tx_start,
        .rx_start = hal_usart2_enable_it_rxne,
        .rx_stop = hal_usart2_disable_it_rxne,
        .read_byte = hal_usart2_read_byte,
};

int drv_init(void)
{
    drv_ain_sensor_init(&s_ain_sensor);
    g_drv.ain_sensor = &s_ain_sensor;

    drv_curr_fdbk_init(&s_curr_fdbk);
    g_drv.curr_fdbk = &s_curr_fdbk;

    uart_control_init(&s_uart, &s_uart_hal_ops,
                      s_uart_rx_buf, UART_RX_BUF_SIZE,
                      5u,
                      NULL, NULL);
    g_drv.uart = &s_uart;

    hal_tim7_start();

    return 0;
}
INIT_PREV_EXPORT(drv_init);

#if 1 /// ain_sensor

static rt_thread_t drv_ain_sensor_thread_ptr;

void drv_ain_sensor_task_entry(void *parameter)
{
    while (1)
    {
        drv_ain_sensor_poll(g_drv.ain_sensor);

        rt_thread_mdelay(DRV_AIN_SENSOR_TASK_PERIOD);
    }
}

int drv_ain_sensor_task_init(void)
{
    drv_ain_sensor_thread_ptr = rt_thread_create("sensor",
                                                 drv_ain_sensor_task_entry,
                                                 NULL,
                                                 DRV_AIN_SENSOR_TASK_STACK_SIZE,
                                                 10,
                                                 25);

    if (drv_ain_sensor_thread_ptr != RT_NULL)
        rt_thread_startup(drv_ain_sensor_thread_ptr);

    return 0;
}
INIT_DEVICE_EXPORT(drv_ain_sensor_task_init);

#endif

#if 1 /// curr_fdbk

static rt_thread_t drv_curr_fdbk_thread_ptr;

void drv_curr_fdbk_task_entry(void *parameter)
{
    while (1)
    {
        drv_curr_fdbk_poll(g_drv.curr_fdbk);

        rt_thread_mdelay(DRV_CURR_FDBK_TASK_PERIOD);
    }
}

int drv_curr_fdbk_task_init(void)
{
    drv_curr_fdbk_thread_ptr = rt_thread_create("curr_fdbk",
                                                drv_curr_fdbk_task_entry,
                                                NULL,
                                                DRV_CURR_FDBK_TASK_STACK_SIZE,
                                                10,
                                                25);

    if (drv_curr_fdbk_thread_ptr != RT_NULL)
        rt_thread_startup(drv_curr_fdbk_thread_ptr);

    return 0;
}
INIT_DEVICE_EXPORT(drv_curr_fdbk_task_init);

#endif
