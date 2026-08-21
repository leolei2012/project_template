#ifndef BL_DOWNLOAD_H
#define BL_DOWNLOAD_H

#include <stdint.h>
#include "bl.h"

/**
 * @file    bl_download.h
 * @brief   下载引擎（传输无关）：收块 → 攒页写 flash → CRC 校验 → copy → commit
 *
 * 适配器（Modbus/YMODEM/...）只需按顺序调用 start/block/end；
 * 主循环周期调用 poll 驱动 copy/commit（重活不阻塞中断）。
 *
 * 块大小任意（len 为参数）；内部按 BL_PAGE_SIZE 攒页 + 懒擦（攒满一页擦一页）。
 */

/** 开始下载：记录期望 size/crc，复位引擎 */
bl_err_t bl_download_start(uint32_t total_size, uint32_t crc32);

/** 接收一块数据 */
bl_err_t bl_download_block(uint32_t block_no, const uint8_t *data, uint32_t len);

/** 结束下载：flush 末页 + 整图 CRC 校验（双槽校验通过后触发 copy） */
bl_err_t bl_download_end(void);

/** 周期调用：驱动 copy/commit（双槽） */
void bl_download_poll(void);

/** 下载进度：已收字节 / 总字节 */
void bl_download_get_progress(uint32_t *received, uint32_t *total);

#endif /* BL_DOWNLOAD_H */
