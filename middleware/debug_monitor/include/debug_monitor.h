#ifndef DEBUG_MONITOR_H
#define DEBUG_MONITOR_H

#include <stdint.h>

#include "modbus/modbus_slave.h"
#include "uart_control.h"

/**
 * @file    debug_monitor.h
 * @brief   debug_monitor 库：设备侧 Modbus 调试接口（基于 uart_control）
 *
 * 让设备通过 Modbus RTU 从站暴露调试寄存器（状态 / 参数 / 进度），供
 * ProbeStation 平台（砺台）或 AI agent 通过 Modbus 读 / 写来调试设备。
 *
 * 设计（uart_control + adapter）：
 *   - 传输：依赖 uart_control（实例由上层注入），内部绑定 RX / TX 回调
 *     （dm_on_rx_frame / dm_on_tx_done），负责收发帧与重启接收；
 *   - 寄存器映射：使用 dm_adapter 提供的调试寄存器表（状态段 + 调试段）；
 *   - 适配器：使用 dm_adapter 的 init / poll 钩子（注册 FC41 等自定义功能码、
 *     周期刷新调试寄存器）。
 */

/** 适配器钩子：在 init / poll 时扩展调试能力 */
typedef struct
{
    void (*init)(struct mb_slave_handle *slave);  /**< mb_slave_init 之后调用（注册 IAP 回调等） */
    void (*poll)(void);                           /**< 周期调用（驱动引擎 + 刷新调试寄存器） */
} dm_adapter;

/** debug_monitor 调试从站对象（单实例） */
typedef struct
{
    struct mb_slave_handle slave;  /**< 内嵌 Modbus 从站 */
    struct uart_control    *uart;  /**< uart_control 传输实例（上层注入） */
} debug_monitor;

/**
 * @brief 初始化调试接口（绑定 uart_control 传输 + dm_adapter 寄存器/适配器）
 * @param self       对象实例
 * @param slave_addr Modbus 从站地址（如 0x01）
 * @param uart       uart_control 实例（由上层注入，如 g_drv.uart）
 */
void debug_monitor_init(debug_monitor *self, uint8_t slave_addr, struct uart_control *uart);

/** 处理收到的一帧（uart_control RX 回调内部调用） */
void debug_monitor_handle_frame(debug_monitor *self, const uint8_t *frame, uint8_t len);

/** 周期调用（驱动适配器的 poll 钩子） */
void debug_monitor_poll(debug_monitor *self);

#endif /* DEBUG_MONITOR_H */
