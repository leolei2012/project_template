# Changelog

本文件记录 STM32G474VET6 数字电源控制器项目的所有变更，按时间倒序排列。

格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/)，
版本号遵循 [Semantic Versioning](https://semver.org/lang/zh-CN/)。

---

## [Unreleased]

### Changed
- **工程定位改为开发板模板**：`AGENTS.md` / `CLAUDE.md` 标题由"数字电源控制器固件项目"改为"STM32G474VET6 开发板模板工程"；`docs/template_README.md` 新增"当前平台"章节（STM32G474VET6 + Keil MDK-ARM）

### Added
- **foc_hfi 脉振高频注入观测器** (`utils/src/foc/foc_hfi.c`)：零速/低速无传感器控制，利用凸极效应探测转子位置，包含 Park 变换、HPF、同相解调、LPF、PLL、Vd 注入值输出
- **HFI 观测器接入 drv_motor** (`drivers/src/motor/drv_motor.c`)：HFI 初始化、ISR 运行、注入值写入、角度选择、闭环切换 HFI 路径、闭环保护 HFI 模式
- **HFI 监控变量** (`middleware/src/monitor/monitor.c`)：新增 6 个 HFI 寄存器（HFI_THETA, HFI_SPEED, HFI_DEMOD_LPF, HFI_IQ_FILT, HFI_VD_INJ, HFI_CONVERGED），用于静止调试 HFI 时观测内部信号
- **vd_inj 注入叠加** (`utils/src/foc/foc_ctrl.c`)：foc_ctrl_run() 中叠加 vd_inj 到 Vd（PI 之后，Circle Limitation 之前）
- **git 仓库初始化**：git init + 初始提交（全部文件）
- **CHANGELOG.md**：变更日志文件

### Changed
- **motor_control 模块剥离**：将纯 FOC/SMO/HFI 算法从 drv_motor 解耦为独立模块 (`utils/src/motor_control/`)，drv_motor 退化为薄封装（~220 行）
- **drv_motor 简化** (`drivers/include/motor/drv_motor.h`)：drv_motor_t 简化为 mc + boot_cap_tick + stop_tick
- **monitor 字段适配** (`middleware/src/monitor/monitor.c`)：g_drv.motor->xxx → g_drv.motor->mc.xxx
- **IncludePath 更新** (`.vscode/c_cpp_properties.json`)：新增 foc, motor_control, motor 路径
- **motor_control Doxygen 注释** (`utils/src/motor_control/`)：全部函数和结构体字段补全 Doxygen 标签（@brief, @param, @retval, @name 分组）

