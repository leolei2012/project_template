#ifndef BL_CONFIG_H
#define BL_CONFIG_H

/**
 * @file    bl_config.h
 * @brief   bootloader 库编译期配置（每个产品/芯片一份）
 *
 * 分区地址须与链接脚本、芯片 flash 布局一致，且页对齐。
 */

/** ================================================================
   OTA 模式
   ================================================================ */
#define BL_OTA_MODE_DUAL_SLOT     0  /**< 双槽：下到 DL_ADDR → 校验 → copy 到 APP */
#define BL_OTA_MODE_SINGLE_SLOT   1  /**< 单槽：直下 APP（无 copy） */
#define BL_OTA_MODE               BL_OTA_MODE_DUAL_SLOT

/** ================================================================
   OTA 运行侧
   ================================================================ */
#define BL_OTA_SIDE_BOOT          0  /**< bootloader 侧：下载 → copy → commit(APP_VALID) → reset */
#define BL_OTA_SIDE_APP           1  /**< APP 侧：下载 → 写 META(IAP_REQUEST) → reset（copy 交给 bootloader） */
#define BL_OTA_SIDE               BL_OTA_SIDE_APP

/** ================================================================
   跳转前 APP 校验开关
   ================================================================ */
#define BL_APP_CHECK_ENABLE        1  /**< 1=跳转前完整校验 APP(magic+size+CRC)；0=只看 magic+标志(跳过 CRC，调试用) */
#define BL_DIRECT_JUMP_APP          0  /**< 1=上电直接跳 APP1（不读 META/不校验/不检查，调试用）；0=走正常 boot_check */

/** ================================================================
   分区（页对齐）
   ================================================================ */
#define BL_PAGE_SIZE              0x800u  /**< 页擦除粒度（2 KB） */

#define BL_APP_ADDR               0x08008000u  /**< 运行槽（固件唯一链接地址） */
#define BL_APP_SIZE               0x39800u  /**< 230 KB */

#if (BL_OTA_MODE == BL_OTA_MODE_DUAL_SLOT)
#define BL_DL_ADDR                0x08041800u  /**< 下载槽（只存字节，不执行） */
#define BL_DL_SIZE                0x39800u
#else   /* SINGLE_SLOT：直接下到运行槽 */
#define BL_DL_ADDR                BL_APP_ADDR
#define BL_DL_SIZE                BL_APP_SIZE
#endif

#define BL_META_ADDR              0x0807F800u  /**< META 页（2 KB） */

/** ================================================================
   META magic（产品标识）
   ================================================================ */
#define BL_META_MAGIC             0x4D435031u  /**< "MCP1" */

#endif /* BL_CONFIG_H */