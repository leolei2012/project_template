# third_party

本目录存放第三方库，分两类：

- **子模块（git submodule）**：独立仓库，主仓库只钉死某个 commit，可整体复用、独立升级。
- **vendor（直接拷贝）**：无独立仓库或暂未迁移，直接拷代码 + 记录版本号。

## 库清单

| 目录                | 类型   | 远端仓库                                       | 说明                                     |
| ------------------- | ------ | ---------------------------------------------- | ---------------------------------------- |
| `uart_control`    | 子模块 | https://github.com/leolei2012/uart_control.git | 通用 UART 收发模块（芯片无关，ops 注入） |
| `mb_rtu`          | 子模块 | https://github.com/leolei2012/mb_rtu.git       | Modbus RTU 主 / 从协议栈                 |
| `log.c`           | 子模块 | https://github.com/rxi/log.c.git               | 轻量日志库                               |
| `bootloader`      | vendor | —（待提供）                                   | IAP 引导 / OTA 下载引擎                  |
| `filter`          | vendor | —（待提供）                                   | 滤波（EMA 等）                           |
| `FlashDB-2.2.0`   | vendor | —（待提供）                                   | 键值数据库                               |
| `littlefs-2.11.3` | vendor | —（待提供）                                   | 嵌入式文件系统                           |
| `pid`             | vendor | —（待提供）                                   | PID 控制器                               |

## 子模块使用

```bash
# 首次克隆（连带拉取子模块）
git clone --recurse-submodules <主仓库>

# 已克隆后补拉
git submodule update --init --recursive

# 更新某个子模块到远端最新 commit
git submodule update --remote third_party/uart_control
```

改动子模块：**在子模块目录内 commit + push 到它自己的仓库**，再回到主仓库 `git add <子模块路径>` 记录新 SHA 并提交。

## 待办

- `bootloader` / `filter` / `FlashDB-2.2.0` / `littlefs-2.11.3` / `pid` 尚无独立远端仓库，待提供 URL 后迁移为子模块；在此之前保持 vendor。
