# ProbeStation 调试指南（Agent 速查手册）

> 本文告诉 AI agent 如何用 **ProbeStation 平台**（PC 侧，砺台）调试设备：读/写寄存器、查状态、OTA 升级。
> 设备侧对应固件模块是 `debug_monitor`（本目录），通过 Modbus 暴露调试寄存器。

---

## 1. 体系概览

调试由两个组件配合完成：

| 组件 | 位置 | 作用 |
|---|---|---|
| **debug_monitor** | 设备固件 `middleware/debug_monitor` | Modbus RTU 从站，暴露调试寄存器 |
| **ProbeStation** | PC（砺台），端口 8080 / 8081 | 轮询 + 存储 + MCP 工具接口 |

- **8080**：Web UI + REST API + WebSocket
- **8081**：MCP 服务器（给 AI agent 的工具接口）

设备侧依赖链：`debug_monitor → modbus(mb_rtu) → uart_control`（本目录 README 有依赖链说明）。

---

## 2. 调试链路

```
agent（你）
  │  MCP 工具调用（8081，streamable-http）
  ▼
ProbeStation 平台
  ├─ poller   ：按寄存器组周期轮询设备（FC03 读）
  ├─ store    ：latest 快照 + 时序历史
  └─ MCP 工具 ：20 个（读/写/查/CRUD/OTA/健康）
      │  Modbus RTU（9600 8N1，从站地址 0x01）
      ▼
设备 debug_monitor（固件，middleware/debug_monitor）
```

关键点：**agent 不直接碰串口**，全程通过 MCP 工具读 store 里的 latest 快照；poller 负责真正轮询设备。

---

## 3. 快速上手（最小 3 步）

MCP 端点是 `http://127.0.0.1:8081/mcp`（streamable-http 协议）。

用 SDK 连接（Node）：

```js
const { Client } = require('@modelcontextprotocol/sdk/client/index.js');
const { StreamableHTTPClientTransport } = require('@modelcontextprotocol/sdk/client/streamableHttp.js');
const t = new StreamableHTTPClientTransport(new URL('http://127.0.0.1:8081/mcp'));
const c = new Client({ name: 'agent', version: '1.0' });
await c.connect(t);
```

三步上手：

1. **列设备** → `list_devices`，拿到 `device_id`。
2. **连接设备** → `set_device_active { device_id, active: true }`（打开串口 + 开始轮询）。
3. **读寄存器** → `get_device_snapshot { device_id }`，返回全部寄存器实时值。

```js
const r = await c.callTool({ name: 'get_device_snapshot', arguments: { device_id: 8 } });
console.log(JSON.parse(r.content[0].text));  // [{register_id, alias, address, data_type, value, timestamp, quality}, ...]
```

---

## 4. MCP 工具速查（20 个）

### 读 / 查

| 工具 | 参数 | 说明 |
|---|---|---|
| `list_devices` | — | 列出所有设备（id/name/ip/port/isActive） |
| `list_registers` | `device_id` | 列出某设备的寄存器定义 |
| `read_register` | `device_id, register_id` | 读单个寄存器实时值 |
| `get_device_snapshot` | `device_id` | 读全部寄存器快照（含地址/类型/时间戳/质量） |
| `query_history` | `device_id, register_id, start, end` | 查寄存器时序历史 |
| `get_device_health` | `device_id` | 健康检查（连接/轮询/最后采样时间/寄存器数） |
| `list_alarm_rules` | `device_id?` | 告警规则 |

### 写 / 控制

| 工具 | 参数 | 说明 |
|---|---|---|
| `write_register` | `device_id, register_id, value` | 写单个寄存器（FC16，控制真机，危险） |
| `set_device_active` | `device_id, active` | 连接/断开设备 |

### 配置（CRUD）

| 工具 | 参数 | 说明 |
|---|---|---|
| `create_group` | `device_id, name, start_address, quantity` | 建寄存器组（自动按地址生成寄存器） |
| `update_group` | `group_id, ...` | 改寄存器组 |
| `delete_group` | `group_id` | 删寄存器组 |
| `create_register` | `group_id, alias, start_address, data_type` | 建寄存器 |
| `update_register` | `register_id, ...` | 改寄存器 |
| `delete_register` | `register_id` | 删寄存器 |

### OTA 升级

| 工具 | 参数 | 说明 |
|---|---|---|
| `upload_firmware` | `name, version, content` | 上传固件（返回 firmware_id） |
| `list_firmwares` | — | 固件列表 |
| `ota_upgrade` | `device_id, firmware_id, chunk_size?` | 发起升级（FC41） |
| `ota_status` | `device_id` | 升级进度（state/block/percent） |
| `ota_abort` | `device_id` | 中止升级 |

---

## 5. 调试寄存器清单（bootloader 示例）

### 状态寄存器（0x0000~0x0004，FC03 读）

| 地址 | 名称 | 含义 |
|---|---|---|
| 0x0000 | state | 0=IDLE 1=RECEIVING 2=VERIFYING 3=COPYING 4=COMMITTING 5=DONE 6=ERROR |
| 0x0001 | boot_reason | 0=APP有效 1=APP无效 2=强制IAP 3=无程序 |
| 0x0002 | app_valid | 0=无效 1=有效 |
| 0x0003 | dl_valid | 0=无 1=有完整镜像 |
| 0x0004 | error | 0=无 1=CRC 2=范围 3=FLASH 4=状态 |

### 调试寄存器（0xE000~0xE016，只读，给调试 agent）

| 地址 | 名称 | 含义 |
|---|---|---|
| 0xE000 | dbg_app_addr | APP 运行槽地址（期望 0x08008000） |
| 0xE002 | dbg_app_size | APP 大小（期望 0x39800=230KB） |
| 0xE004 | dbg_dl_addr | 下载槽地址（期望 0x08041800） |
| 0xE006 | dbg_dl_size | 下载槽大小 |
| 0xE008 | dbg_meta_addr | META 页地址（期望 0x0807F800） |
| 0xE00A | dbg_page_size | flash 页大小（期望 0x800=2KB） |
| 0xE00B | dbg_meta_magic | META magic（期望 0x4D435031="MCP1"） |
| 0xE00D | dbg_ota_mode | OTA 模式 0=双槽 1=单槽 |
| 0xE00E | dbg_meta_app_size | META 记录的 APP 长度 |
| 0xE010 | dbg_meta_app_crc32 | META 记录的 APP CRC32 |
| 0xE012 | dbg_meta_flags | META 标志位（bit0=app_valid, bit1=iap_request） |
| 0xE013 | dbg_dl_received | 下载已收字节 |
| 0xE015 | dbg_dl_total | 下载总字节 |

> 32 位字段是「高 16 在前，低 16 在后」（大端），如 `dbg_app_addr = (0xE000 << 16) | 0xE001`。
> 完整语义清单见 `docs/ProbeStation/bootloader.yaml`。

---

## 6. 典型调试场景

### 6.1 判断设备在哪（读状态）

```js
const snap = await c.callTool({ name: 'get_device_snapshot', arguments: { device_id: 8 } });
// 看 address=0 (state)、address=1 (boot_reason)
// state=0 + reason=3 → bootloader 在 IAP 等下载
// state=5 → 刚完成 OTA，即将跳 APP
```

### 6.2 验证分区是否正确（读调试寄存器）

读 0xE000 段，核对 app_addr=0x08008000、dl_addr=0x08041800、magic="MCP1"，用于排查「链接地址/分区配置」类问题。

### 6.3 OTA 升级流程

1. `upload_firmware` → 拿 `firmware_id`。
2. `ota_upgrade { device_id, firmware_id }`。
3. 轮询 `ota_status` 直到 `state=done`（或 `failed`）。
4. 升级成功设备复位，跳 APP。

### 6.4 读历史 / 健康

- `query_history`：看某个寄存器的历史曲线（排查抖动/异常）。
- `get_device_health`：看连接/轮询是否正常、最后采样时间。

---

## 7. 踩坑记录（重要）

1. **agent 不直接开串口**：串口被 ProbeStation 的 poller 独占，外部直连会 `Access denied`。
2. **RTU 半双工**：同一串口多台从站共享一个 SerialDriver，按 `serialPath` 串行轮询。
3. **int16 有符号**：调试寄存器用 `int16` 显示，如 0x8000 会显示成 -32768；32 位值要自己拼 `(高<<16)|低`。
4. **寄存器要先建组**：`get_device_snapshot` 只返回「已配置的寄存器组」覆盖的地址；调试段要先 `create_group`（start_address=0xE000, quantity=23）。
5. **写寄存器是 FC16 控制真机**，危险操作，谨慎使用。
6. **RTU 停用不释放串口**（ProbeStation 设计如此），要释放得重启平台或删设备。

---

## 8. 一句话总结

**agent 调试 = MCP 三件套**：`list_devices` 找设备 → `set_device_active` 连接 → `get_device_snapshot` 读寄存器；需要升级走 `ota_upgrade`，需要控制走 `write_register`。


---

## 9. 附录：OTA 测试手册（双槽，实测通过 2026-08）

> 从零烧录 → 下发升级 → 验证 的一整套可复现步骤。优先用 8080 REST（一行 Invoke-RestMethod），
> 不必走 8081 MCP 的 JSON-RPC 握手（那套要自己管 session-id + SSE，麻烦）。

### 9.1 分区与两条升级路径

| 区域 | 地址 | 大小 |
|---|---|---|
| Bootloader | 0x08000000 | 32 KB |
| APP1（运行槽） | 0x08008000 | 230 KB（0x39800） |
| APP2（下载槽） | 0x08041800 | 230 KB |
| NV | 0x0807B000 | 18 KB |
| META | 0x0807F800 | 2 KB |

META 结构（bl_meta_t，64 字节）：magic("MCP1") + app_size + app_crc32 + flags + reserved[12]。
flags：bit0=APP_VALID，bit1=IAP_REQUEST。

路径 A — bootloader 侧升级（设备停在 bootloader，META 空/无有效 APP）：
FC41 下发 → 写 APP2 → CRC 校验 → copy APP2→APP1 → 写 META(APP_VALID) → 复位 → 跳 APP

路径 B — APP 侧升级（设备正常跑 APP，运行中升级）：
FC41 下发 → 写 APP2 → CRC 校验 → 写 META(IAP_REQUEST) → 复位
   → bootloader 上电见 IAP_REQUEST → apply_pending：copy APP2→APP1 → 写 META(APP_VALID) → 复位 → 跳新 APP

固件侧开关：bl_config.h 的 BL_OTA_SIDE（BOOT / APP），两边分别编译。

### 9.2 前置：烧录（J-Link 命令行）

J-Link Commander（C:\Program Files\SEGGER\JLink_V960\JLink.exe）：

flash_bl.jlink —— 烧 bootloader：
connect
loadbin bootloader.hex, 0x08000000
r
g
exit

flash_app.jlink —— 烧 APP：
connect
loadbin stm32g474vet6_app.bin, 0x08008000
r
g
exit

执行：
& "C:\Program Files\SEGGER\JLink_V960\JLink.exe" -Device STM32G474VE -If SWD -Speed 4000 -AutoConnect 1 -CommanderScript flash_app.jlink

### 9.3 测试步骤（8080 REST）

平台先启动（tsx apps/cli/src/main.ts），默认 8080 REST + 8081 MCP + 8502 模拟从站。

$BASE = "http://127.0.0.1:8080"

# 1. 确认设备（device_id=8）已配置并连接
Invoke-RestMethod "$BASE/api/monitor_objects"
Invoke-RestMethod "$BASE/api/monitor_objects/8/latest"

# 2. 上传固件（原始二进制 body，不用 base64）
Invoke-RestMethod -Method Post -ContentType "application/octet-stream" -Body ([IO.File]::ReadAllBytes("stm32g474vet6_app.bin")) "$BASE/api/firmware/upload?name=ota_app&version=0.2.0"

# 3. 发起升级（异步，返回 task_id）
Invoke-RestMethod -Method Post -ContentType "application/json" -Body '{"device_id":8,"firmware_id":6}' "$BASE/api/ota/upgrade"

# 4. 轮询状态直到 done/failed
Invoke-RestMethod "$BASE/api/ota/status?device_id=8"

# 5. 升级完成后确认设备跳到新 APP
Invoke-RestMethod "$BASE/api/monitor_objects/8/latest"

### 9.4 期望结果与判读

- ota_status：transferring → 百分比涨到 100% → done。失败会停在某 block + failed + error。
- 判读设备在哪（看快照 address=0/1）：
  - 设备在 bootloader：0x0000=state(0=IDLE)、0x0001=boot_reason(0=APP有效,3=无程序)。
  - 设备在 APP：0x0000 段变成 FOC 数据（母线电压/电流持续刷新），0xE000 调试段时间戳停在 bootloader 最后一次刷新。
- 升级成功后 bootloader 的 boot_reason 从 3(无程序) 变 0(APP有效)。

### 9.5 踩坑记录（本次实测的关键 bug）

1. APP 侧 flash 写不能放在中断上下文：FC41 DATA 回调在 TIM7 超时中断里，bl_download_block 攒满一页直接写 flash
   会失败（擦除成功、编程失败，卡在第一个满页块，约 block 15）。修复：攒页入队列，flash 写挪到 bl_download_poll()
   （middleware 线程上下文）。
2. port_flash_erase/program 不能 __disable_irq：HAL_FLASH 的 FLASH_WaitForLastOperation 用 HAL_GetTick() 做超时判断，
   __disable_irq 屏蔽了 SysTick 导致编程失败。双 bank RWW 下中断本就不干扰 flash 写，直接去掉。
3. 串口 stream destroyed：J-Link 复位/烧录后 ProbeStation 串口连接失效，报 Cannot call write after a stream was destroyed。
   处理：set_device_active(active=false) 再 active=true 重连串口。
4. 固件版本号：两个 main.c 都放 const uint32_t g_version = 0x010000u;（主.次.修订），当前只是占位，未接入 META/寄存器。

### 9.6 一句话总结

测 OTA = 烧录（J-Link）→ REST 上传固件 → POST /api/ota/upgrade → 轮询 GET /api/ota/status → 看快照确认跳 APP。
