#include "bl.h"
#include "bl_meta.h"

#include <string.h>

static const bl_port_t *s_port;
static bl_status_t     s_status;

/** ================================================================
   端口访问（供 bl_meta / bl_download 使用）
   ================================================================ */
const bl_port_t *bl_port(void)
{
    return s_port;
}

/** ================================================================
   状态 setter（供 bl_download.c 更新，内部接口）
   ================================================================ */
void bl_status_set_state(bl_state_t state) { s_status.state = state; }
void bl_status_set_error(bl_err_t err)      { s_status.err   = err;   }
void bl_status_set_dl_valid(bool valid)     { s_status.dl_valid = valid; }

/** ================================================================
   公共 API
   ================================================================ */
void bl_init(const bl_port_t *port)
{
    s_port = port;

    memset(&s_status, 0, sizeof(s_status));
    s_status.state  = BL_STATE_IDLE;
    s_status.reason = BL_REASON_NO_APP;
    s_status.err    = BL_OK;
}

void bl_boot_check(void)
{
#if BL_DIRECT_JUMP_APP
    /** 调试模式：上电直接跳 APP1，跳过一切校验与检查（bl_jump_to_app 不返回） */
    bl_jump_to_app(BL_APP_ADDR);
    /** 若 APP 为空跳转失败返回，继续走下面的 IAP 兜底 */
#endif

    bl_meta_t meta;
    bl_meta_read(&meta);

    /** 先验 magic，再读 flags —— 空 META(全 0xFF) 时 flags 也全 1，不能直接当 IAP_REQUEST */
    bool meta_valid = (meta.magic == BL_META_MAGIC);
#if BL_APP_CHECK_ENABLE
    bool app_valid  = meta_valid && bl_meta_app_valid(&meta);  /**< 完整校验：magic + size + CRC */
#else
    bool app_valid  = meta_valid && ((meta.flags & BL_FLAG_APP_VALID) != 0u);  /**< 跳过 CRC，只看标志 */
#endif
    bool iap_req    = meta_valid && ((meta.flags & BL_FLAG_IAP_REQUEST) != 0u);
    bool force      = (s_port != NULL && s_port->force_iap != NULL && s_port->force_iap());

    s_status.app_valid = app_valid;

    if (app_valid && !iap_req && !force)
    {
        bl_jump_to_app(BL_APP_ADDR);  /**< 不返回 */
    }

    /** 进 IAP */
    s_status.state     = BL_STATE_IDLE;
    s_status.err       = BL_OK;
    s_status.dl_valid  = false;
    s_status.reason    = !meta_valid ? BL_REASON_NO_APP
                       : (force || iap_req) ? BL_REASON_IAP_REQUEST
                       : BL_REASON_APP_INVALID;
}

bl_status_t bl_get_status(void)
{
    return s_status;
}