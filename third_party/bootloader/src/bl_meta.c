#include "bl_meta.h"
#include "bl_config.h"
#include "bl_port.h"
#include "bl_crc.h"

#include <string.h>

void bl_meta_read(bl_meta_t *m)
{
    if (m != NULL)
    {
        /** flash 内存映射，直接读 */
        memcpy(m, (const void *)BL_META_ADDR, sizeof(*m));
    }
}

bool bl_meta_write(const bl_meta_t *m)
{
    const bl_port_t *p = bl_port();

    if (m == NULL || p == NULL)
    {
        return false;
    }

    if (!p->flash_erase(BL_META_ADDR, BL_PAGE_SIZE))
    {
        return false;
    }

    /** sizeof(bl_meta_t)=64，按 flash 编程宽度对齐 */
    return p->flash_program(BL_META_ADDR, (const uint8_t *)m, sizeof(*m));
}

bool bl_meta_app_valid(const bl_meta_t *m)
{
    if (m == NULL)
    {
        return false;
    }

    if (m->magic != BL_META_MAGIC)
    {
        return false;
    }

    if (m->app_size == 0u || m->app_size > BL_APP_SIZE)
    {
        return false;
    }

    uint32_t crc = bl_crc32((const uint8_t *)BL_APP_ADDR, m->app_size);

    return crc == m->app_crc32;
}