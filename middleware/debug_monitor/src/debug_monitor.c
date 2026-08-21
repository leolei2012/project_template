#include "debug_monitor.h"
#include "dm_adapter.h"
#include "uart_control.h"

#include <string.h>

static dm_adapter s_adapter;

/** 发送回调：ctx 为注册时透传的 debug_monitor 实例 */
static uint8_t dm_uart_send(const uint8_t *buf, uint8_t len, void *ctx)
{
    debug_monitor *self = (debug_monitor *)ctx;
    return (uart_control_send(self->uart, buf, len) == UART_CONTROL_OK) ? len : 0u;
}

/** RX 完成回调：通过 uart->ctx 找回实例 */
static void dm_on_rx_frame(struct uart_control *uart, uint16_t len)
{
    debug_monitor *self = (debug_monitor *)uart->ctx;

    if (len == 0u)
    {
        uart_control_enable_rx(uart);
        return;
    }

    dm_adapter_refresh();
    debug_monitor_handle_frame(self, uart->rx_buf, (uint8_t)len);

    uart_control_enable_rx(uart);
}

/** TX 完成回调：重启接收 */
static void dm_on_tx_done(struct uart_control *uart)
{
    uart_control_enable_rx(uart);
}

void debug_monitor_init(debug_monitor *self, uint8_t slave_addr, struct uart_control *uart)
{
    const dm_adapter *adapter;

    if (self == NULL || uart == NULL)
    {
        return;
    }

    self->uart = uart;

    adapter = dm_adapter_get();
    if (adapter != NULL)
    {
        s_adapter = *adapter;
    }
    else
    {
        memset(&s_adapter, 0, sizeof(s_adapter));
    }

    memset(&self->slave, 0, sizeof(self->slave));

    mb_slave_init(&self->slave, slave_addr, dm_adapter_reg_map());
    self->slave.base.send = dm_uart_send;
    self->slave.base.ctx  = self;          /**< 发送回调上下文 */

    if (s_adapter.init != NULL)
    {
        s_adapter.init(&self->slave);
    }

    uart_control_set_ctx(uart, self);      /**< RX / TX 回调上下文 */
    uart_control_bind_rx_done_callback(uart, dm_on_rx_frame);
    uart_control_bind_tx_done_callback(uart, dm_on_tx_done);
    uart_control_enable_rx(uart);
}

void debug_monitor_handle_frame(debug_monitor *self, const uint8_t *frame, uint8_t len)
{
    if (self == NULL || frame == NULL || len == 0u)
    {
        return;
    }

    mb_slave_rx_frame(&self->slave, frame, len);
}

void debug_monitor_poll(debug_monitor *self)
{
    (void)self;

    if (s_adapter.poll != NULL)
    {
        s_adapter.poll();
    }
}
