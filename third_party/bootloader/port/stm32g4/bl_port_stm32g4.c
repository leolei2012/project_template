#include "bl_port_stm32g4.h"
#include "bl_config.h"

#include "stm32g4xx_hal.h"
#include "stm32g4xx_ll_tim.h"
#include "stm32g4xx_ll_usart.h"

/** ================================================================
   flash 驱动（基于 HAL_FLASH，2KB 页擦 + 64-bit 编程）
   ================================================================ */

#define G4_FLASH_BASE    0x08000000u
#define G4_FLASH_END     0x08080000u  /**< 512 KB */

static bool s_unlocked = false;

static bool port_flash_erase(uint32_t addr, uint32_t len)
{
    if (!s_unlocked)
    {
        return false;
    }

    /** 保护 bootloader 区（< BL_APP_ADDR 一律拒绝） */
    if (addr < BL_APP_ADDR || (addr + len) > G4_FLASH_END)
    {
        return false;
    }

    if ((addr & (BL_PAGE_SIZE - 1u)) != 0u)
    {
        return false;
    }

    if (len == 0u)
    {
        return true;
    }

    uint32_t page;
    uint32_t banks;
    uint32_t nb = (len + BL_PAGE_SIZE - 1u) / BL_PAGE_SIZE;

    /** STM32G4 支持双 bank（DBANK 选项位）：页号是 bank 内页号，Bank2 从 0x08040000 开始 */
    if ((FLASH->OPTR & FLASH_OPTR_DBANK) != 0U)
    {
        if (addr >= 0x08040000u)
        {
            banks = FLASH_BANK_2;
            page  = (addr - 0x08040000u) / BL_PAGE_SIZE;
        }
        else
        {
            banks = FLASH_BANK_1;
            page  = (addr - G4_FLASH_BASE) / BL_PAGE_SIZE;
        }
    }
    else
    {
        banks = FLASH_BANK_1;
        page  = (addr - G4_FLASH_BASE) / BL_PAGE_SIZE;
    }

    FLASH_EraseInitTypeDef erase = {0};
    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.Banks     = banks;
    erase.Page      = page;
    erase.NbPages   = nb;

    uint32_t err = 0u;

    HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&erase, &err);

    return status == HAL_OK;
}

static bool port_flash_program(uint32_t addr, const uint8_t *buf, uint32_t len)
{
    if (!s_unlocked)
    {
        return false;
    }

    if (buf == NULL)
    {
        return false;
    }

    if (addr < BL_APP_ADDR || (addr + len) > G4_FLASH_END)
    {
        return false;
    }

    /** 64-bit（8 字节）对齐 */
    if ((addr & 0x7u) != 0u || (len & 0x7u) != 0u)
    {
        return false;
    }

    for (uint32_t i = 0u; i < len; i += 8u)
    {
        uint64_t word = 0u;
        for (uint32_t j = 0u; j < 8u; j++)
        {
            word |= ((uint64_t)buf[i + j]) << (j * 8u);
        }

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr + i, word) != HAL_OK)
        {
            return false;
        }
    }

    return true;
}

/** ================================================================
   复位 / 反初始化 / 触发 IAP
   ================================================================ */

static void port_reset(void)
{
    NVIC_SystemReset();
}

static void port_deinit(void)
{
    /** 关闭并清除 bootloader 用到的中断（防止跳转后残留 pending 触发 APP 中断，访问未初始化的 g_drv 空指针） */
    NVIC_DisableIRQ(USART2_IRQn);
    NVIC_ClearPendingIRQ(USART2_IRQn);
    NVIC_DisableIRQ(TIM7_DAC_IRQn);
    NVIC_ClearPendingIRQ(TIM7_DAC_IRQn);
    NVIC_DisableIRQ(DMA1_Channel2_IRQn);
    NVIC_ClearPendingIRQ(DMA1_Channel2_IRQn);

    /** 复位外设，确保 APP 拿到的是复位态（而非 bootloader 留下的运行态） */
    LL_TIM_DisableCounter(TIM7);
    LL_TIM_ClearFlag_UPDATE(TIM7);
    LL_USART_Disable(USART2);
    LL_USART_ClearFlag_ORE(USART2);

    HAL_FLASH_Lock();
    s_unlocked = false;
}

static bool port_force_iap(void)
{
    /** 本板无独立按键，返回 false；需要时在此接 GPIO（如长按某键） */
    return false;
}

/** ================================================================
   跳转 APP（Cortex-M 通用，此处用设备头提供的 CMSIS）
   ================================================================ */
void bl_jump_to_app(uint32_t app_base)
{
    uint32_t sp = *(volatile uint32_t *)app_base;  /**< APP 初始 SP */
    uint32_t pc = *(volatile uint32_t *)(app_base + 4u);  /**< Reset_Handler */

    port_deinit();  /**< 复位外设 + 锁 flash */

    __disable_irq();

    /** 关 SysTick 并清内核 pending：避免跳转后 APP 开中断时被 bootloader 残留的 SysTick/PendSV 打断 */
    SysTick->CTRL = 0u;
    SysTick->LOAD = 0u;
    SysTick->VAL  = 0u;
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk | SCB_ICSR_PENDSVCLR_Msk;

    SCB->VTOR = app_base;

    /** 一次性切换 MSP 并跳转：sp/pc 必须保持在寄存器里。
     *  若分开写 __set_MSP(sp) 再 ((void(*)(void))pc)()，编译器可能把 pc spill
     *  到旧栈帧，__set_MSP 之后再用新 MSP 读 [sp+offset] 会读到垃圾地址。 */
    __ASM volatile (
        "MSR MSP, %0\n"
        "BX  %1\n"
        : : "r" (sp), "r" (pc) : "memory"
    );

    /** 不会执行到这里 */
    while (1)
    {
    }
}

/** ================================================================
   端口实例
   ================================================================ */

static const bl_port_t s_port =
{
    .flash_erase   = port_flash_erase,
    .flash_program = port_flash_program,
    .reset         = port_reset,
    .deinit        = port_deinit,
    .force_iap     = port_force_iap,
};

void bl_port_stm32g4_init(void)
{
    if (HAL_FLASH_Unlock() == HAL_OK)
    {
        s_unlocked = true;
    }
}

const bl_port_t *bl_port_stm32g4_get(void)
{
    return &s_port;
}