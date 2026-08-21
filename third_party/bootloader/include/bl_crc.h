#ifndef BL_CRC_H
#define BL_CRC_H

#include <stdint.h>

/**
 * @file    bl_crc.h
 * @brief   CRC32（标准 CRC-32，poly 0xEDB88320，初值/终值 0xFFFFFFFF）
 */

uint32_t bl_crc32(const uint8_t *data, uint32_t len);

#endif /* BL_CRC_H */
