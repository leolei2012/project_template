#include "bl_download.h"
#include "bl_meta.h"
#include "bl_crc.h"
#include "bl_port.h"

#include <string.h>

/** ================================================================
   下载引擎状态
   ================================================================ */
static bl_state_t s_state = BL_STATE_IDLE;

static uint32_t s_expected_size;
static uint32_t s_expected_crc32;
static uint32_t s_dl_offset;  /**< 已写入下载槽的字节数 */
static uint32_t s_page_fill;  /**< 攒页缓冲已填字节数 */

static uint8_t s_page_buf[BL_PAGE_SIZE];    /**< 攒页缓冲（中断上下文填充） */
static uint8_t s_pending_buf[BL_PAGE_SIZE]; /**< 待写页缓冲（线程上下文写 flash） */
static uint32_t s_pending_addr;             /**< 待写页地址 */
static bool s_pending_valid;                /**< 是否有待写页 */

#if (BL_OTA_MODE == BL_OTA_MODE_DUAL_SLOT) && (BL_OTA_SIDE == BL_OTA_SIDE_BOOT)
static uint8_t s_copy_buf[BL_PAGE_SIZE];  /**< copy 搬运用（仅双槽 bootloader 侧） */
#endif

static void bl_set_state(bl_state_t s)
{
    s_state = s;
    bl_status_set_state(s);
}

/** 擦一页 + 编程一页 */
static bool bl_flash_write_page(uint32_t page_addr, const uint8_t *data)
{
    const bl_port_t *p = bl_port();

    if (p == NULL)
    {
        return false;
    }

    if (!p->flash_erase(page_addr, BL_PAGE_SIZE))
    {
        return false;
    }

    return p->flash_program(page_addr, data, BL_PAGE_SIZE);
}

/**
 * 攒满一页后，把页复制到待写缓冲，交给 poll 在「线程上下文」写 flash。
 * 不能在中断上下文（FC41 DATA 回调）里写 flash：擦写会关中断几十 ms，
 * 且 HAL_FLASH 依赖 SysTick 的超时机制，中断上下文里会失败。
 */
static bool bl_queue_page(uint32_t addr, uint32_t fill)
{
    if (s_pending_valid)
    {
        return false;  /**< 上一页还没写完（poll 太慢） */
    }

    memcpy(s_pending_buf, s_page_buf, fill);
    memset(&s_pending_buf[fill], 0xFF, BL_PAGE_SIZE - fill);

    s_pending_addr = addr;
    s_pending_valid = true;
    s_page_fill = 0u;

    return true;
}

/** ================================================================
   下载引擎 API
   ================================================================ */

bl_err_t bl_download_start(uint32_t total_size, uint32_t crc32)
{
    if (s_state != BL_STATE_IDLE)
    {
        return BL_ERR_STATE;
    }

    if (total_size == 0u || total_size > BL_DL_SIZE)
    {
        return BL_ERR_RANGE;
    }

    s_expected_size  = total_size;
    s_expected_crc32 = crc32;
    s_dl_offset      = 0u;
    s_page_fill      = 0u;
    s_pending_valid  = false;

    bl_set_state(BL_STATE_RECEIVING);
    bl_status_set_error(BL_OK);
    bl_status_set_dl_valid(false);

    return BL_OK;
}

bl_err_t bl_download_block(uint32_t block_no, const uint8_t *data, uint32_t len)
{
    (void)block_no;  /**< 块号连续性由适配器/协议保证 */

    if (s_state != BL_STATE_RECEIVING)
    {
        return BL_ERR_STATE;
    }

    if (data == NULL || len == 0u)
    {
        return BL_ERR_RANGE;
    }

    if ((s_dl_offset + len) > BL_DL_SIZE)
    {
        return BL_ERR_RANGE;
    }

    uint32_t idx = 0u;

    while (idx < len)
    {
        uint32_t space = BL_PAGE_SIZE - s_page_fill;
        uint32_t n     = (len - idx) < space ? (len - idx) : space;

        memcpy(&s_page_buf[s_page_fill], &data[idx], n);
        s_page_fill += n;
        s_dl_offset += n;
        idx         += n;

        if (s_page_fill == BL_PAGE_SIZE)
        {
            uint32_t addr = BL_DL_ADDR + (s_dl_offset - BL_PAGE_SIZE);

            if (!bl_queue_page(addr, BL_PAGE_SIZE))
            {
                bl_set_state(BL_STATE_ERROR);
                bl_status_set_error(BL_ERR_FLASH);
                return BL_ERR_FLASH;
            }
        }
    }

    return BL_OK;
}

bl_err_t bl_download_end(void)
{
    if (s_state != BL_STATE_RECEIVING)
    {
        return BL_ERR_STATE;
    }

    /** flush 末页（不足一页用 0xFF 补齐），交给 poll 写 */
    if (s_page_fill > 0u)
    {
        uint32_t addr = BL_DL_ADDR + (s_dl_offset - s_page_fill);

        if (!bl_queue_page(addr, s_page_fill))
        {
            bl_set_state(BL_STATE_ERROR);
            bl_status_set_error(BL_ERR_FLASH);
            return BL_ERR_FLASH;
        }
    }

    if (s_dl_offset != s_expected_size)
    {
        bl_set_state(BL_STATE_IDLE);
        bl_status_set_error(BL_ERR_RANGE);
        return BL_ERR_RANGE;
    }

    /** 进 VERIFYING，交给 poll：写末页 + 整图 CRC 校验 */
    bl_set_state(BL_STATE_VERIFYING);

    return BL_OK;
}

/** ================================================================
   copy + commit（双槽的 copy 只在 bootloader 侧编译）
   ================================================================ */

#if (BL_OTA_MODE == BL_OTA_MODE_DUAL_SLOT) && (BL_OTA_SIDE == BL_OTA_SIDE_BOOT)
static bool bl_copy_dl_to_app(void)
{
    for (uint32_t off = 0u; off < s_expected_size; off += BL_PAGE_SIZE)
    {
        uint32_t n = s_expected_size - off;
        if (n > BL_PAGE_SIZE)
        {
            n = BL_PAGE_SIZE;
        }

        memset(s_copy_buf, 0xFF, sizeof(s_copy_buf));
        memcpy(s_copy_buf, (const void *)(BL_DL_ADDR + off), n);  /**< 读下载槽 */

        if (!bl_flash_write_page(BL_APP_ADDR + off, s_copy_buf))
        {
            return false;
        }
    }

    return true;
}
#endif

static bool bl_commit_app(void)
{
    bl_meta_t meta;
    bl_meta_read(&meta);
    meta.magic      = BL_META_MAGIC;
    meta.app_size   = s_expected_size;

#if (BL_OTA_SIDE == BL_OTA_SIDE_APP)
    /** APP 侧：下载槽(APP2) 已就绪，只写 IAP_REQUEST 标记，copy 交给 bootloader */
    meta.app_crc32  = bl_crc32((const uint8_t *)BL_DL_ADDR, s_expected_size);
    meta.flags      = BL_FLAG_IAP_REQUEST;
#else
    /** bootloader 侧：已 copy 到运行槽(APP1)，标记 APP_VALID */
    meta.app_crc32  = bl_crc32((const uint8_t *)BL_APP_ADDR, s_expected_size);
    meta.flags      = BL_FLAG_APP_VALID;  /**< 同时清 IAP_REQUEST */
#endif

    return bl_meta_write(&meta);
}

void bl_download_poll(void)
{
    /** 1. 写待写页（线程上下文写 flash，不阻塞中断） */
    if (s_pending_valid)
    {
        if (!bl_flash_write_page(s_pending_addr, s_pending_buf))
        {
            bl_set_state(BL_STATE_ERROR);
            bl_status_set_error(BL_ERR_FLASH);
            s_pending_valid = false;
            return;
        }
        s_pending_valid = false;
    }

    /** 2. VERIFYING：所有页写完后做整图 CRC 校验 */
    if (s_state == BL_STATE_VERIFYING)
    {
        if (s_pending_valid)
        {
            return;  /**< 还有页没写完 */
        }

        uint32_t crc = bl_crc32((const uint8_t *)BL_DL_ADDR, s_expected_size);

        if (crc != s_expected_crc32)
        {
            bl_set_state(BL_STATE_IDLE);
            bl_status_set_error(BL_ERR_CRC);
            bl_status_set_dl_valid(false);
            return;
        }

        bl_status_set_dl_valid(true);

#if (BL_OTA_SIDE == BL_OTA_SIDE_APP)
        bl_set_state(BL_STATE_COMMITTING);  /**< APP 侧直接写 IAP_REQUEST，不 copy */
#elif (BL_OTA_MODE == BL_OTA_MODE_DUAL_SLOT)
        bl_set_state(BL_STATE_COPYING);  /**< 交给 poll 做 copy */
#else
        bl_set_state(BL_STATE_COMMITTING);  /**< 单槽直接 commit */
#endif
    }

    /** 3. copy（仅 bootloader 侧双槽） */
#if (BL_OTA_MODE == BL_OTA_MODE_DUAL_SLOT) && (BL_OTA_SIDE == BL_OTA_SIDE_BOOT)
    if (s_state == BL_STATE_COPYING)
    {
        if (!bl_copy_dl_to_app())
        {
            bl_set_state(BL_STATE_ERROR);
            bl_status_set_error(BL_ERR_FLASH);
            return;
        }
        bl_set_state(BL_STATE_COMMITTING);
    }
#endif

    /** 4. commit */
    if (s_state == BL_STATE_COMMITTING)
    {
        if (!bl_commit_app())
        {
            bl_set_state(BL_STATE_ERROR);
            bl_status_set_error(BL_ERR_FLASH);
            return;
        }

        bl_status_set_dl_valid(false);
        bl_set_state(BL_STATE_DONE);

        const bl_port_t *p = bl_port();
        if (p != NULL && p->reset != NULL)
        {
            p->reset();  /**< 复位重跑 boot_check → 跳 APP */
        }
    }
}

void bl_download_get_progress(uint32_t *received, uint32_t *total)
{
    if (received != NULL)
    {
        *received = s_dl_offset;
    }
    if (total != NULL)
    {
        *total = s_expected_size;
    }
}
