# bootloader 库

芯片无关的 bootloader 逻辑库（纯 C，无 HAL/RTOS 依赖，无动态内存）。
把「该不该跳 APP」「怎么收固件」「怎么校验」「怎么 copy」「怎么提交」这些
与芯片无关的流程做成库；换芯片只需实现 `bl_port_t` 一组端口函数。

## 结构

```
bootloader/
├── include/    bl.h, bl_config.h, bl_port.h, bl_meta.h, bl_download.h, bl_crc.h
├── src/        bl.c, bl_meta.c, bl_download.c, bl_crc.c
├── port/stm32g4/       参考端口（flash/reset/deinit/force_iap）
└── adapter/modbus/     Modbus FC41 适配器（调 bl_download_* + 状态寄存器）
```

## 层次

```
adapter (Modbus/YMODEM/...)  ── 调 bl_download_start/block/end
        │
        ▼
引擎 bl_download / bl_meta / bl_crc  （芯片无关）
        │
        ▼
端口 bl_port_t  （flash 擦写 / reset / deinit / force_iap）
```

## 特性

- 双槽 copy（下到 DL 槽 → 校验 → copy APP）与单槽流式（直下 APP）两种模式，宏切换。
- 上电校验 APP（magic + size + CRC32）再跳转；空 META（首次生产）自动进 IAP。
- 校验只依赖 magic + size + CRC32，无版本号。
- 状态快照 `bl_get_status()` 供适配器映射到状态寄存器（可观测）。
- 不做：run-confirm 回滚、flash_read 抽象、跨断电续传。
