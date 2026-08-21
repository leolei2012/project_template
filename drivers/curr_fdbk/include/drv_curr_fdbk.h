#ifndef DRV_CURR_FDBK_H
#define DRV_CURR_FDBK_H

#include "platform.h"

/*
 *  相电流采样驱动
 *
 *  拓扑选择: 修改 DRV_CURR_FDBK_TOPOLOGY 宏
 *    DRV_CURR_FDBK_2SHUNT  — 两电阻 (A/B 相下桥臂，永不切换)
 *    DRV_CURR_FDBK_3SHUNT  — 三电阻 (R3_2, 双 ADC 动态 JSQR)
 *
 *  ADC1 注入: JDR1=CH7(Iu,PC1), JDR2=CH8(Iv,PC2)
 *  ADC2 注入: JDR1=CH8(Iv,PC2), JDR2=CH9(Iw,PC3)
 *  TIM1 TRGO=OC4REF 同步触发两路 ADC
 *
 *  I_mA = i_s16 * 27500 >> 16
 */

/** —— 拓扑选择 —— */
#define DRV_CURR_FDBK_2SHUNT  2u
#define DRV_CURR_FDBK_3SHUNT  3u
#define DRV_CURR_FDBK_TOPOLOGY  DRV_CURR_FDBK_3SHUNT

/** —— 硬件参数 —— */
#define CURR_RSHUNT_OHM            0.02f
#define CURR_AMPLIFICATION_GAIN    6.0f
#define CURR_ADC_REF_MV            3300u
#define CURR_MAX_READABLE_MA       ((int32_t)13750)

/** —— 扇区定义 (3-shunt 专用) —— */
#if DRV_CURR_FDBK_TOPOLOGY == DRV_CURR_FDBK_3SHUNT
#define CURR_SECTOR_1  0u
#define CURR_SECTOR_2  1u
#define CURR_SECTOR_3  2u
#define CURR_SECTOR_4  3u
#define CURR_SECTOR_5  4u
#define CURR_SECTOR_6  5u

/** JSQR 单通道注入模板 */
#define CURR_JSQR_TEMPLATE(ch)  \
    ((uint32_t)((uint32_t)(ch) << ADC_JSQR_JSQ1_Pos) \
   | (LL_ADC_INJ_TRIG_EXT_TIM1_TRGO & ~ADC_INJ_TRIG_EXT_EDGE_DEFAULT))

/** —— 校准状态 —— */
#define CURR_CALIB_STATE_IDLE         0u
#define CURR_CALIB_STATE_PHASE_AB     1u  /**< Sector 5: 采 Ia, Ib */
#define CURR_CALIB_STATE_PHASE_C      2u  /**< Sector 1: 采 Ic */
#define CURR_CALIB_SAMPLES            256u
#endif

/** ============================================================
   单相电流数据
   ============================================================ */
struct drv_curr_fdbk_channel
{
    int16_t  i_s16;  /**< offset-corrected ADC */
    float    current_A;  /**< 物理电流 (A), ISR 中实时更新 */
    uint16_t adc_raw;
    int32_t  current_mA;
    uint16_t offset;
    uint16_t gain_factor;  /**< i_s16 → mA (Q16.16, 默认 27500) */
};

/** ============================================================
   三相电流反馈顶层结构
   ============================================================ */
struct drv_curr_fdbk
{
    struct drv_curr_fdbk_channel ia;
    struct drv_curr_fdbk_channel ib;
    struct drv_curr_fdbk_channel ic;
#if DRV_CURR_FDBK_TOPOLOGY == DRV_CURR_FDBK_3SHUNT
    uint8_t  sector;
    uint8_t  active_sector;  /**< 本拍 ADC 采样所用扇区 (TIM1_UP 从 pending 提升) */
    uint8_t  pending_sector;  /**< 下拍 JSQR 所用扇区 (TIM1_UP 新写入) */
#endif
    bool     calibrated;

    /** 校准状态 (参考 MCSDK R3_2_CurrentReadingPolarization) */
#if DRV_CURR_FDBK_TOPOLOGY == DRV_CURR_FDBK_3SHUNT
    uint8_t  calib_state;
    uint16_t calib_count;
    uint16_t calib_tick;   /**< 非阻塞校准的 1ms 节拍计数 */
    uint32_t calib_sum_a;
    uint32_t calib_sum_b;
    uint32_t calib_sum_c;
#endif
};

/** ============================================================
   API
   ============================================================ */
void drv_curr_fdbk_init(struct drv_curr_fdbk *cb);

#if DRV_CURR_FDBK_TOPOLOGY == DRV_CURR_FDBK_3SHUNT
/** 非阻塞启动校准 (Phase AB, 随后 Phase C 由 _poll 推进) */
void drv_curr_fdbk_calibrate_start(struct drv_curr_fdbk *cb);
/** 非阻塞推进校准 (1kHz 调用), 返回 true 表示校准完成 */
bool drv_curr_fdbk_calibrate_poll(struct drv_curr_fdbk *cb);
void drv_curr_fdbk_tim1_up_isr(struct drv_curr_fdbk *cb);
#endif

void drv_curr_fdbk_adc_isr(struct drv_curr_fdbk *cb);

#define DRV_CURR_FDBK_TASK_PERIOD 1u
void drv_curr_fdbk_poll(struct drv_curr_fdbk *cb);

#endif /* DRV_CURR_FDBK_H */