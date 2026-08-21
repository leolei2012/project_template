#include "dm_adapter.h"

#include "bl_download.h"

/** ---------- Modbus 测试寄存器映射 ---------- */

/** 只读测试段（0xE000 起）：魔数 / 版本 / 自增计数 */
enum test_reg_t
{
    TEST_REG_MAGIC = 0,   /**< 固定魔数 0x1234（验证读链路） */
    TEST_REG_VERSION,     /**< 版本号 0x0100 */
    TEST_REG_COUNTER_L,   /**< 自增计数器低 16 位 */
    TEST_REG_COUNTER_H,   /**< 自增计数器高 16 位（32-bit 计数） */
    TEST_REG_NUM,
};

static uint16_t s_test_regs[TEST_REG_NUM];

/** 可写测试段（0x0000 起）：写回显 */
enum ctrl_reg_t
{
    CTRL_REG_TEST_WORD = 0,  /**< 测试字：写什么读回什么 */
    CTRL_REG_TEST_CMD,       /**< 测试命令 */
    CTRL_REG_NUM,
};

static uint16_t s_ctrl_regs[CTRL_REG_NUM] = { 0x0000u, 0x0000u };

/** 写回调：测试字直接回显 */
static enum mb_err_t ctrl_on_write(uint16_t addr, uint16_t val)
{
    switch (addr)
    {
    case CTRL_REG_TEST_WORD:
        s_ctrl_regs[CTRL_REG_TEST_WORD] = val;
        return MB_OK;

    case CTRL_REG_TEST_CMD:
        s_ctrl_regs[CTRL_REG_TEST_CMD] = val;
        return MB_OK;

    default:
        return MB_ERR_ADDR;
    }
}

static struct mb_reg_map s_reg_map =
{
    .coils    = {{0, 0, NULL}},
    .coils_num = 0,

    .discrete    = {{0, 0, NULL}},
    .discrete_num = 0,

    .holding = {
        {
            .start_addr = 0x0000,   /**< 测试写段 */
            .num        = CTRL_REG_NUM,
            .data       = s_ctrl_regs,
            .on_write   = ctrl_on_write,
        },
        {
            .start_addr = 0xE000,   /**< 测试读段 */
            .num        = TEST_REG_NUM,
            .data       = s_test_regs,
        },
    },
    .holding_num = 2,

    .input    = {{0, 0, NULL}},
    .input_num = 0,
};

static uint32_t s_counter = 0u;

void dm_adapter_refresh(void)
{
    s_counter++;

    s_test_regs[TEST_REG_MAGIC]     = 0x1234u;
    s_test_regs[TEST_REG_VERSION]   = 0x0100u;
    s_test_regs[TEST_REG_COUNTER_L] = (uint16_t)(s_counter & 0xFFFFu);
    s_test_regs[TEST_REG_COUNTER_H] = (uint16_t)((s_counter >> 16u) & 0xFFFFu);
}

/** ---------- 适配器钩子 ---------- */

/** FC41 IAP 回调：对接 bootloader 下载引擎（APP 侧只下载到 APP2 + 写 IAP_REQUEST，copy 交给 bootloader） */
static enum mb_err_t iap_on_start(uint32_t total_size, uint32_t crc32)
{
    return (bl_download_start(total_size, crc32) == BL_OK) ? MB_OK : MB_ERR_RANGE;
}

static enum mb_err_t iap_on_data(uint16_t block_no, const uint8_t *data, uint8_t len)
{
    return (bl_download_block((uint32_t)block_no, data, (uint32_t)len) == BL_OK)
           ? MB_OK : MB_ERR_RANGE;
}

static enum mb_err_t iap_on_end(void)
{
    return (bl_download_end() == BL_OK) ? MB_OK : MB_ERR_CRC;
}

static void adapter_init(struct mb_slave_handle *slave)
{
    if (slave != NULL)
    {
        slave->iap.on_start = iap_on_start;
        slave->iap.on_data  = iap_on_data;
        slave->iap.on_end   = iap_on_end;
    }

    dm_adapter_refresh();
}

static void adapter_poll(void)
{
    bl_download_poll();  /**< 驱动 APP 侧 commit(写 IAP_REQUEST) + reset */
}

static const dm_adapter s_adapter =
{
    .init = adapter_init,
    .poll = adapter_poll,
};

const struct mb_reg_map *dm_adapter_reg_map(void)
{
    return &s_reg_map;
}

const dm_adapter *dm_adapter_get(void)
{
    return &s_adapter;
}
