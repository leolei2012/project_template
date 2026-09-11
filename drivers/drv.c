#include "drv.h"

#include "hal.h"

struct drv g_drv;

static struct drv_ain_sensor s_ain_sensor;

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

    uart_control_init(&s_uart, &s_uart_hal_ops,
                      s_uart_rx_buf, UART_RX_BUF_SIZE,
                      5u,
                      NULL, NULL);
    g_drv.uart = &s_uart;

    hal_tim7_start();

    return 0;
}
INIT_PREV_EXPORT(drv_init);

static rt_thread_t drv_ain_sensor_thread_ptr;

void drv_ain_sensor_task_entry(void *parameter)
{
    (void)parameter;

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
    {
        rt_thread_startup(drv_ain_sensor_thread_ptr);
    }

    return 0;
}
INIT_DEVICE_EXPORT(drv_ain_sensor_task_init);
