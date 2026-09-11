# STM32G474VET6 开发板模板工程

基于 **STM32G474VET6**（Arm Cortex-M4F @ 170 MHz）的嵌入式固件模板工程，采用 RT-Thread + Keil MDK-ARM，按分层架构组织，可作为新产品 / 新板卡的起点骨架。

## 特性

- **分层架构**：`bsp → hal → drivers → middleware → app`，依赖单向无环，换板只改 `bsp/`
- **RT-Thread**：多线程 + `INIT_*_EXPORT` 自动分级初始化
- **可复用模块集**：
  - 驱动：模拟量传感器（ADC + EMA 滤波）
  - 中间件：`debug_monitor`（Modbus RTU 调试接口）
  - 应用示例：`alarm_system` 告警系统
  - 工具：alarm / check_algorithm / debug / ntc_temp / platform / rcm / ring_buffer
  - 第三方：bootloader（IAP/OTA）、filter、pid、mb_rtu（Modbus）、uart_control
- **规范落地**：目录形态 C、分布式组合根、构造注入，详见 `docs/规范/`

## 硬件平台

| 项 | 说明 |
|---|---|
| MCU | STM32G474VET6（Cortex-M4F，170 MHz） |
| 原理图 / 手册 | `docs/原理图/`（DMG474、ATK-PD6010B） |
| 数据手册 | `docs/datasheet/` |

## 目录结构

```text
app/         应用层（业务逻辑，不直接操作硬件）
drivers/     驱动层（外设封装）
hal/         HAL 抽象层（外设实例薄封装）
bsp/         板级支持包（入口 / 时钟 / 引脚 / NVIC / 启动 / 链接脚本）
middleware/  中间件（debug_monitor）
config/      项目配置
utils/       工具模块
tests/       单元测试 & 集成测试
third_party/ 第三方库
project/     Keil 工程
rtos/        RT-Thread
docs/        文档与规范
```

## 构建

### 前置条件

1. **Keil MDK-ARM**（μVision 5）
2. **mcal（厂商 SDK）**：`mcal/` 被 `.gitignore` 忽略，需手动放置：
   - STM32Cube_FW_G4_V1.6.0 → `mcal/STM32Cube_FW_G4_V1.6.0/`
   - CubeMX 配置 → `mcal/cubemx/`
3. **子模块**：
   ```bash
   git submodule update --init --recursive
   ```

### 编译

用 Keil 打开 `project/MDK-ARM/project.uvprojx` 后编译即可。

## 快速上手

固件入口在 `bsp/src/main.c`（当前为 LED 闪烁示例）。各层初始化顺序由 RT-Thread 的 `INIT_*_EXPORT` 自动编排：

```text
hal_init        (INIT_BOARD_EXPORT)   板级 / 外设初始化
drv_init        (INIT_PREV_EXPORT)    驱动实例化 + 注入
middleware_init (INIT_ENV_EXPORT)     中间件初始化
app_init        (INIT_ENV_EXPORT)     应用实例化 + 注入
<层>_task_init  (INIT_DEVICE/APP)     建线程
```

## 编码约定

- 层次依赖单向：上层可调用下层，反之不行；应用层不直接碰 STM32 HAL/LL
- 对象用带 tag 的 `struct`，方法为「自由函数 + `self`」，依赖走 `init` 构造注入
- 详见 `docs/规范/architecture.md` 与 `docs/规范/coding_standard.md`

## 作者

见 [AUTHORS](./AUTHORS)。

## 变更记录

见 [CHANGELOG.md](./CHANGELOG.md)。
