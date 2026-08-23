/** Includes ------------------------------------------------------------------ */
#include "bsp_isr.h"
/** Private includes ---------------------------------------------------------- */
#include "stm32g4xx_hal.h"
#include "stm32g4xx_ll_adc.h"
#include "stm32g4xx_ll_tim.h"
#include "stm32g4xx_ll_usart.h"
#include "hal_usart2.h"
#include "hal_tim1.h"
#include "drv.h"

extern void rt_os_tick_callback(void);
extern void idle_led_callback(void);

volatile uint32_t g_adc_isr_count = 0;
volatile uint32_t g_tim1_up_isr_count = 0;

/* ================================================================
 * 故障诊断捕获 (排查"跑飞"用)
 *
 * 异常进入时 CPU 已把 R0-R3/R12/LR/PC/xPSR 压入当前栈(MSP/PSP),
 * 这里按 EXC_RETURN(LR) bit2 选栈, 把帧 + 各故障状态寄存器存到全局,
 * 供 J-Link 在 while(1) 停留期间读取 (地址见 .map 文件)。
 * ================================================================ */
#define FAULT_SCB_CFSR   (*(volatile uint32_t *)0xE000ED28u)
#define FAULT_SCB_HFSR   (*(volatile uint32_t *)0xE000ED2Cu)
#define FAULT_SCB_MMFAR  (*(volatile uint32_t *)0xE000ED34u)
#define FAULT_SCB_BFAR   (*(volatile uint32_t *)0xE000ED38u)

#define FAULT_ID_NONE     0u
#define FAULT_ID_MM       1u
#define FAULT_ID_BUS      2u
#define FAULT_ID_USAGE    3u
#define FAULT_ID_HARD     4u

volatile uint32_t g_fault_id;
volatile uint32_t g_fault_cfsr;
volatile uint32_t g_fault_hfsr;
volatile uint32_t g_fault_mmfar;
volatile uint32_t g_fault_bfar;
volatile uint32_t g_fault_pc;
volatile uint32_t g_fault_lr;
volatile uint32_t g_fault_xpsr;
volatile uint32_t g_fault_sp;

static uint32_t *fault_get_frame(void)
{
    uint32_t *frame;
    __asm volatile
    (
        "TST LR, #4\n\t"
        "ITE EQ\n\t"
        "MRSEQ %0, MSP\n\t"
        "MRSNE %0, PSP\n\t"
        : "=r" (frame)
        :
        : "cc"
    );
    return frame;
}

static void fault_capture(uint32_t id)
{
    uint32_t *frame = fault_get_frame();

    g_fault_id    = id;
    g_fault_cfsr  = FAULT_SCB_CFSR;
    g_fault_hfsr  = FAULT_SCB_HFSR;
    g_fault_mmfar = FAULT_SCB_MMFAR;
    g_fault_bfar  = FAULT_SCB_BFAR;
    g_fault_pc    = frame[6];
    g_fault_lr    = frame[5];
    g_fault_xpsr  = frame[7];
    g_fault_sp    = (uint32_t)frame;

    /* 安全关断 PWM 输出, 防止故障后占空比冻结继续驱动电机 */
    LL_TIM_DisableAllOutputs(TIM1);

    __disable_irq();
    while (1)
    {
    }
}

/******************************************************************************/
/** Cortex-M4 Processor Interruption and Exception Handlers */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
   while (1)
  {
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  fault_capture(FAULT_ID_MM);
}

/**
  * @brief This function handles Prefetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  fault_capture(FAULT_ID_BUS);
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  fault_capture(FAULT_ID_USAGE);
}

/* 注意: HardFault_Handler 由 RT-Thread 的 context_rvds.S 定义(会打印 PC/LR/PSR),
 * 不要在此重复定义。 */

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  HAL_IncTick();
  rt_os_tick_callback();

}

/******************************************************************************/
/** STM32G4xx Peripheral Interrupt Handlers */
/** Add here the Interrupt Handlers for the used peripherals. */
/** For the available peripheral interrupt handler names, */
/** please refer to the startup file (startup_stm32g4xx.s). */
/******************************************************************************/

/**
  * @brief ADC1/ADC2 共享中断
  *
  * 注入组 JEOC：TIM1 触发的三相电流采样
  * 规则组 EOC：不再使用（软件触发 + poll 阻塞读取）
  */
void ADC1_2_IRQHandler(void)
{
    /** —— ADC1 注入序列转换完成（三相电流采样）—— */
    if (LL_ADC_IsActiveFlag_JEOS(ADC1) != 0U)
    {
        g_adc_isr_count++;
        LL_ADC_ClearFlag_JEOS(ADC1);
        drv_curr_fdbk_adc_isr(g_drv.curr_fdbk);
    }
    else
    {
        /** Spurious entry or other ADC1 source — clear JEOS anyway */
        LL_ADC_ClearFlag_JEOS(ADC1);
    }

    /** —— ADC2 注入序列转换完成（与 ADC1 同步触发，仅清标志）—— */
    if (LL_ADC_IsActiveFlag_JEOS(ADC2) != 0U)
    {
        LL_ADC_ClearFlag_JEOS(ADC2);
    }
}

/**
  * @brief TIM1 UP / TIM16 共享中断
  *
  * TIM1 更新中断（每个 PWM 周期，中心对齐下溢 + 上溢）：
  *   重配置 ADC1/ADC2 JSQR 为当前扇区对应的单通道注入序列，
  *   然后使能 TIM1_TRGO=OC4REF 触发下一轮 ADC 注入转换。
  */
void TIM1_UP_TIM16_IRQHandler(void)
{
#if DRV_CURR_FDBK_TOPOLOGY == DRV_CURR_FDBK_3SHUNT
    if (LL_TIM_IsActiveFlag_UPDATE(TIM1) != 0U)
    {
        g_tim1_up_isr_count++;
        LL_TIM_ClearFlag_UPDATE(TIM1);
        drv_curr_fdbk_tim1_up_isr(g_drv.curr_fdbk);
    }
#endif
}

/**
  * @brief TIM6 / DAC 共享中断
  *
  * TIM6 提供固定 1kHz 的传感器 ADC 采样节拍，
  * 在 ISR 中完成软件触发 + 阻塞读取 + EMA 滤波。
  */
void TIM6_DAC_IRQHandler(void)
{
    if (LL_TIM_IsActiveFlag_UPDATE(TIM6) != 0U)
    {
        LL_TIM_ClearFlag_UPDATE(TIM6);
        drv_ain_sensor_tim_isr(g_drv.ain_sensor);
    }
}

/**
  * @brief TIM7 / DAC 共享中断
  *
  * TIM7 空闲超时定时器：收到字节时复位，超时 = 帧接收完成。
  */
void TIM7_DAC_IRQHandler(void)
{
    if (LL_TIM_IsActiveFlag_UPDATE(TIM7) != 0U)
    {
        LL_TIM_ClearFlag_UPDATE(TIM7);
        uart_control_timer_isr(g_drv.uart);
    }
}

/**
  * @brief USART2 中断（RXNE 逐字节接收）
  */
void USART2_IRQHandler(void)
{
    /** 处理上溢错误：ORE 未清除会阻止后续 RXNE 中断 */
    if (LL_USART_IsActiveFlag_ORE(USART2) != 0U)
    {
        LL_USART_ClearFlag_ORE(USART2);
    }

    if (LL_USART_IsActiveFlag_RXNE(USART2) != 0U)
    {
        uart_control_rx_isr(g_drv.uart);
    }
}

/**
  * @brief DMA1 CH2 中断（USART2_TX 发送完成）
  */
void DMA1_Channel2_IRQHandler(void)
{
    hal_usart2_dma_tx_isr();
    uart_control_tx_done_isr(g_drv.uart);
}
