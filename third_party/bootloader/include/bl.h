#ifndef BL_H
#define BL_H

#include <stdint.h>
#include <stdbool.h>

#include "bl_config.h"
#include "bl_port.h"

/**
 * @file    bl.h
 * @brief   bootloader 库公共接口：类型 + 入口 + 状态
 *
 * 用法：
 *   bl_init(&port);       //// 绑定硬件端口
 *   bl_boot_check();      //// 上电判定：APP 有效 → 跳转（不返回）；否则进 IAP
 *   ...                   //// 适配器驱动 bl_download_*，主循环调 bl_download_poll()
 *   bl_get_status();      //// 查询状态，供适配器映射到状态寄存器
 */

/** ================================================================
   状态机
   ================================================================ */
typedef enum {
    BL_STATE_IDLE = 0,  /**< 空闲，等 START */
    BL_STATE_RECEIVING,  /**< 接收中 */
    BL_STATE_VERIFYING,  /**< 整图 CRC 校验中 */
    BL_STATE_COPYING,  /**< copy 下载槽 → 运行槽（仅双槽） */
    BL_STATE_COMMITTING,  /**< 提交 META */
    BL_STATE_DONE,  /**< 完成 */
    BL_STATE_ERROR,  /**< 出错 */
} bl_state_t;

/** ================================================================
   进入 bootloader 的原因
   ================================================================ */
typedef enum {
    BL_REASON_APP_VALID   = 0,  /**< APP 有效，正常跳转 */
    BL_REASON_APP_INVALID = 1,  /**< APP CRC/size 校验失败 */
    BL_REASON_IAP_REQUEST = 2,  /**< 用户强制 IAP */
    BL_REASON_NO_APP      = 3,  /**< 空 META（首次生产）/ META 损坏 */
} bl_boot_reason_t;

/** ================================================================
   错误码
   ================================================================ */
typedef enum {
    BL_OK = 0,
    BL_ERR_CRC,  /**< CRC 校验失败 */
    BL_ERR_RANGE,  /**< 长度/地址越界 */
    BL_ERR_FLASH,  /**< flash 操作失败 */
    BL_ERR_STATE,  /**< 状态机错误 */
} bl_err_t;

/** ================================================================
   状态快照（供适配器映射到 Modbus 状态寄存器）
   ================================================================ */
typedef struct {
    bl_state_t       state;
    bl_boot_reason_t reason;
    bool             app_valid;  /**< 运行槽 APP 是否有效 */
    bool             dl_valid;  /**< 下载槽是否有完整已校验镜像 */
    bl_err_t         err;
} bl_status_t;

/** 初始化：绑定硬件端口 */
void bl_init(const bl_port_t *port);

/**
 * @brief 上电判定（BOOT_CHECK）
 * APP 有效且无 IAP 请求 → 直接跳转（不返回）；
 * 否则进 IAP：设 state=IDLE、记 reason，返回后由 main 跑 IAP 循环。
 */
void bl_boot_check(void);

/** 查询状态快照 */
bl_status_t bl_get_status(void);

/** ================================================================
   内部接口（bl_download.c 更新状态用，勿直接调用）
   ================================================================ */
void bl_status_set_state(bl_state_t state);
void bl_status_set_error(bl_err_t err);
void bl_status_set_dl_valid(bool valid);

#endif /* BL_H */