# STM32G474VET6 开发板模板工程

基于 STM32G474VET6 的开发板模板工程。当前保留完整模块集（FOC 电机控制、RT-Thread、Modbus、bootloader、DGUS 屏等）作为可复用骨架，后续按需裁剪业务逻辑。

## 硬件平台
- **MCU**: STM32G474VET6 (Arm Cortex-M4F, 170 MHz)
- **参考文档**: `docs/DMG474_V1.0.pdf` (芯片手册), `docs/ATK-PD6010B_V1.0.pdf` (电源板手册)

## 构建工具链
- **IDE**: Keil MDK-ARM (μVision)
- **工程文件**: `project/MDK-ARM/project.uvprojx`
- **MCAL 版本**: STM32Cube_FW_G4_V1.6.0
- **CubeMX 配置**: `mcal/cubemx/stm32g474vet6/` (主配置), `mcal/cubemx/stm32g474vet6_motor/` (电机配置)

## RTOS
- **主 RTOS**: RT-Thread (位于 `rtos/rt-thread/`)
- **裸机选项**: `rtos/bare/`

## 项目架构

```
app/         应用层：业务逻辑，不直接操作硬件
  ├── alarm_system/   告警系统
  ├── compressor/     压缩机控制
  ├── core/           核心状态机 / 模式管理
  ├── fan/            风机控制
  ├── hmi/            人机界面逻辑
  ├── memory/         数据存储管理
  └── water_pump/     水泵控制
drivers/      驱动层：封装外设操作
  ├── ain_sensor/     模拟量传感器
  ├── buzz/           蜂鸣器
  ├── dgus/           DGUS 串口触摸屏
  ├── inverter/       逆变器驱动
  ├── key/            按键
  ├── memory/         EEPROM / Flash 存储
  └── output/         数字量输出
hal/          HAL 抽象层：外设实例的薄封装 (adc1, tim1, usart2 等)，不含板级细节
bsp/          板级支持包：入口 (main.c)、时钟树、引脚映射、NVIC、启动文件、链接脚本 (与这块 PCB 绑定)
middleware/   中间件：monitor (监控), ota (固件升级), protocol (通信协议)
config/       项目配置 (project_config.h 含 Keil Configuration Wizard 注解)
utils/        工具模块
tests/        单元测试 & 集成测试
third_party/  第三方库 (PY32T090 触摸库)
```

## 编码约定
- **层次依赖**: bsp ← hal ← drivers ← app。上层可以调用下层，反之不行
- 应用层通过 HAL 抽象层访问硬件，不直接调用 STM32 HAL 或 LL
- 换板子时只改 `bsp/`，`hal/` 和 `drivers/` 不应改动
- 系统时钟 170 MHz，所有定时参数基于此频率
- 固件版本号定义在 `config/project_config.h`
