#ifndef BL_META_H
#define BL_META_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @file    bl_meta.h
 * @brief   META 页：APP 元数据 + 标志（64 字节，其余页空间保留）
 */

#define BL_FLAG_APP_VALID    (1u << 0)  /**< APP 已通过校验 */
#define BL_FLAG_IAP_REQUEST  (1u << 1)  /**< 请求进入 IAP */

typedef struct {
    uint32_t magic;  /**< BL_META_MAGIC */
    uint32_t app_size;  /**< APP 固件长度（字节） */
    uint32_t app_crc32;  /**< APP 镜像 CRC32 */
    uint32_t flags;          /**< BL_FLAG_* */
    uint32_t reserved[12];  /**< 保留，sizeof 凑整 64 字节 */
} bl_meta_t;

/** 读 META（flash 内存映射直接读） */
void bl_meta_read(bl_meta_t *m);

/** 写 META（擦 META 页 + 编程，调用方须保证内容合法） */
bool bl_meta_write(const bl_meta_t *m);

/** APP 是否有效（magic + size 范围 + CRC32 全过） */
bool bl_meta_app_valid(const bl_meta_t *m);

#endif /* BL_META_H */