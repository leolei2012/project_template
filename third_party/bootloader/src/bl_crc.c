#include "bl_crc.h"

/** 标准 CRC-32（poly 0xEDB88320，初值 0xFFFFFFFF，输出异或 0xFFFFFFFF） */
uint32_t bl_crc32(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFu;

    for (uint32_t i = 0u; i < len; i++)
    {
        crc ^= data[i];
        for (uint32_t j = 0u; j < 8u; j++)
        {
            if ((crc & 1u) != 0u)
            {
                crc = (crc >> 1u) ^ 0xEDB88320u;
            }
            else
            {
                crc >>= 1u;
            }
        }
    }

    return ~crc;
}
