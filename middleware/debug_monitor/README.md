# debug_monitor

设备侧 **Modbus RTU 调试接口**：通过 Modbus 从站把调试寄存器暴露给上位机 / agent 读 / 写，用于验证 Modbus 库链路、在线调速 / 读状态、以及 FC41 OTA 升级。

## 目录结构

```text
debug_monitor/
├── include/
│   ├── debug_monitor.h      调试从站核心接口
│   └── dm_adapter.h         测试适配器接口
├── src/
│   ├── debug_monitor.c      核心：uart 传输 + Modbus 从站
│   └── dm_adapter.c         适配器：测试寄存器 + FC41 IAP/OTA
├── reg_map.xlsx             寄存器映射表（按地址段分 sheet）
└── ProbeStation_调试指南.md  平台调试完整手册
```

## 职责分工

| 文件 | 职责 |
|---|---|
| `debug_monitor.c` | Modbus 从站 + `uart_control` 传输：绑定 RX/TX 回调、收/发帧、重启接收 |
| `dm_adapter.c` | 数据源适配：暴露测试寄存器（读/写）+ FC41 IAP/OTA 钩子 |

> 依赖倒置：`debug_monitor` 只认 `dm_adapter` 接口，具体数据源（当前是测试寄存器）由适配器提供。

## 接口

见 `debug_monitor.h` / `dm_adapter.h`。核心入口：

- `debug_monitor_init(self, 从站地址, uart)` — 组合根调用，注入 `uart_control` 实例
- `debug_monitor_handle_frame(self, frame, len)` — 收帧处理（内部由 RX 回调调用）
- `debug_monitor_poll(self)` — 周期驱动适配器
- `dm_adapter_reg_map()` / `dm_adapter_get()` / `dm_adapter_refresh()` — 寄存器表 / 钩子 / 刷新

实例化在组合根 `middleware/middleware.c`：

```c
static debug_monitor s_debug_monitor;
debug_monitor_init(&s_debug_monitor, 0x01, g_drv.uart);   // 从站地址 0x01
```

## 寄存器映射

见 [reg_map.xlsx](./reg_map.xlsx)（按地址段分 sheet）。摘要：

| 段 | 地址 | 寄存器 | 读写 | 说明 |
|---|---|---|---|---|
| 写段 | 0x0000 | TEST_WORD | 读/写 | 写什么读回什么 |
| 写段 | 0x0001 | TEST_CMD | 读/写 | 测试命令 |
| 读段 | 0xE000 | MAGIC | 只读 | 魔数 0x1234 |
| 读段 | 0xE001 | VERSION | 只读 | 版本 0x0100 |
| 读段 | 0xE002 | COUNTER_L | 只读 | 自增计数器低16位 |
| 读段 | 0xE003 | COUNTER_H | 只读 | 自增计数器高16位 |

## 依赖

- `mb_rtu`（Modbus 从站）
- `uart_control`（串口收发）
- `bootloader`（FC41 IAP/OTA，在 `dm_adapter.c` 内对接 `bl_download`）

## 测试

1. 烧录后，上位机以 Modbus RTU（9600 8N1，从站地址 0x01）连接设备串口。
2. 读保持寄存器 0xE000 起：`MAGIC`=0x1234，`COUNTER` 每次读递增。
3. 写 0x0000 / 0x0001，再读回值一致 → 读写链路正常。

完整平台调试流程（MCP / REST / OTA）见 [ProbeStation_调试指南.md](./ProbeStation_调试指南.md)。
