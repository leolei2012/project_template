#include "drv_curr_fdbk.h"

#include "drv.h"
#include "hal.h"

/** 饱和到 int16 范围 */
static inline int16_t saturate_int16(int32_t val)
{
    if (val > INT16_MAX)  return INT16_MAX;
    if (val < -INT16_MAX) return (int16_t)(-INT16_MAX);
    return (int16_t)val;
}

/** ============================================================
   3-shunt: JSQR 查找表
   ============================================================ */
#if DRV_CURR_FDBK_TOPOLOGY == DRV_CURR_FDBK_3SHUNT

static const uint32_t s_adc1_jsqr[6] = {
    CURR_JSQR_TEMPLATE(8u),  /**< SECTOR_1: CH8=Iv */
    CURR_JSQR_TEMPLATE(7u),  /**< SECTOR_2: CH7=Iu */
    CURR_JSQR_TEMPLATE(7u),  /**< SECTOR_3: CH7=Iu */
    CURR_JSQR_TEMPLATE(7u),  /**< SECTOR_4: CH7=Iu */
    CURR_JSQR_TEMPLATE(7u),  /**< SECTOR_5: CH7=Iu */
    CURR_JSQR_TEMPLATE(8u),  /**< SECTOR_6: CH8=Iv */
};

static const uint32_t s_adc2_jsqr[6] = {
    CURR_JSQR_TEMPLATE(9u),  /**< SECTOR_1: CH9=Iw */
    CURR_JSQR_TEMPLATE(9u),  /**< SECTOR_2: CH9=Iw */
    CURR_JSQR_TEMPLATE(9u),  /**< SECTOR_3: CH9=Iw */
    CURR_JSQR_TEMPLATE(8u),  /**< SECTOR_4: CH8=Iv */
    CURR_JSQR_TEMPLATE(8u),  /**< SECTOR_5: CH8=Iv */
    CURR_JSQR_TEMPLATE(9u),  /**< SECTOR_6: CH9=Iw */
};

#endif /* DRV_CURR_FDBK_3SHUNT */

/** ============================================================
   初始化
   ============================================================ */
void drv_curr_fdbk_init(struct drv_curr_fdbk *cb)
{
    if (cb == NULL)
    {
        return;
    }

    memset(cb, 0, sizeof(*cb));

#if DRV_CURR_FDBK_TOPOLOGY == DRV_CURR_FDBK_3SHUNT
    cb->sector         = CURR_SECTOR_4;
    cb->active_sector  = CURR_SECTOR_4;
    cb->pending_sector = CURR_SECTOR_4;
#endif

    cb->ia.offset      = 32768u;
    cb->ib.offset      = 32768u;
    cb->ic.offset      = 32768u;

    /** 默认增益系数 = 3300 / 65536 / (Rshunt * Gain) * 65536
       = 3300 / (0.02 * 6) = 27500
       每通道可单独校准 */
    cb->ia.gain_factor = 27500u;
    cb->ib.gain_factor = 27500u;
    cb->ic.gain_factor = 27500u;
}

/** ============================================================
   Offset 校准 (参考 MCSDK R3_2_CurrentReadingPolarization)

   两阶段:
     Phase AB (Sector 5): ADC1=CH7(Iu)→Ia, ADC2=CH8(Iv)→Ib
     Phase C  (Sector 1): ADC2=CH9(Iw)→Ic

   PWM 跑 50% 占空比, 电机无电流, 用注入采样路径采集 N 次取平均。
   ============================================================ */
#if DRV_CURR_FDBK_TOPOLOGY == DRV_CURR_FDBK_3SHUNT
void drv_curr_fdbk_calibrate_start(struct drv_curr_fdbk *cb)
{
    if (cb == NULL) return;

    /** —— Phase AB: 校准 Ia, Ib offset —— */
    cb->calib_state = CURR_CALIB_STATE_PHASE_AB;
    cb->calib_tick  = 0u;
    cb->calib_count = 0u;
    cb->calib_sum_a = 0u;
    cb->calib_sum_b = 0u;
    cb->calib_sum_c = 0u;
    cb->sector      = CURR_SECTOR_5;

    /** 50% 占空比, 三相中点, 无净电压 */
    hal_tim1_set_duty_abc(PWM_HALF_PERIOD / 2u,
                          PWM_HALF_PERIOD / 2u,
                          PWM_HALF_PERIOD / 2u);
    hal_tim1_pwm_enable();
    hal_tim1_start();
}

bool drv_curr_fdbk_calibrate_poll(struct drv_curr_fdbk *cb)
{
    if (cb == NULL) return true;
    if (cb->calib_state == CURR_CALIB_STATE_IDLE) return true;

    cb->calib_tick++;

    switch (cb->calib_state)
    {
    case CURR_CALIB_STATE_PHASE_AB:
        /** 等待 256 次注入采样 (16kHz → ~16ms, 留余量 25ms) */
        if (cb->calib_tick >= 25u)
        {
            hal_tim1_stop();
            hal_tim1_pwm_disable();
            if (cb->calib_count > 0u)
            {
                cb->ia.offset = (uint16_t)(cb->calib_sum_a / cb->calib_count);
                cb->ib.offset = (uint16_t)(cb->calib_sum_b / cb->calib_count);
            }

            /** —— Phase C: 校准 Ic offset —— */
            cb->calib_state = CURR_CALIB_STATE_PHASE_C;
            cb->calib_tick  = 0u;
            cb->calib_count = 0u;
            cb->calib_sum_c = 0u;
            cb->sector      = CURR_SECTOR_1;
            hal_tim1_pwm_enable();
            hal_tim1_start();
        }
        break;

    case CURR_CALIB_STATE_PHASE_C:
        if (cb->calib_tick >= 25u)
        {
            hal_tim1_stop();
            hal_tim1_pwm_disable();
            if (cb->calib_count > 0u)
            {
                cb->ic.offset = (uint16_t)(cb->calib_sum_c / cb->calib_count);
            }
            cb->calib_state = CURR_CALIB_STATE_IDLE;
            cb->calib_tick  = 0u;
            cb->calibrated  = true;
        }
        break;

    default:
        break;
    }

    return (cb->calib_state == CURR_CALIB_STATE_IDLE);
}
#endif

/** ============================================================
   3-shunt: TIM1_UP ISR — 重配置 JSQR + 使能触发
   ============================================================ */
#if DRV_CURR_FDBK_TOPOLOGY == DRV_CURR_FDBK_3SHUNT

void drv_curr_fdbk_tim1_up_isr(struct drv_curr_fdbk *cb)
{
    if (cb == NULL)
    {
        return;
    }

    uint8_t s = cb->sector;

    /* 防跑飞: 等待上一拍注入转换被消费 (JSQR 清零)。上电/校准后 TIM1_OC4REF
     * 触发偶发丢失会使 JSQR 永久非零, 原来的裸 while 会死循环卡死整个系统。
     * 加超时上限, 超时后跳过本轮重配 (下一拍恢复)。 */
    /* 超时上限只兜底"JSQR 永久非零"(ADC 注入转换停摆)。
     * 注意函数调用开销: 1000 次≈150us, 仍>62.5us 周期会饿死调度器; 缩到 100 次≈15us。 */
    uint32_t jsqr_timeout = 100u;
    while ((hal_adc1_inj_is_jsqr_busy() || hal_adc2_inj_is_jsqr_busy())
           && jsqr_timeout > 0u)
    {
        jsqr_timeout--;
    }

    hal_adc1_inj_set_jsqr(s_adc1_jsqr[s] | (uint32_t)LL_ADC_INJ_TRIG_EXT_RISING);
    hal_adc2_inj_set_jsqr(s_adc2_jsqr[s] | (uint32_t)LL_ADC_INJ_TRIG_EXT_RISING);

    /* 重新武装注入转换(JADSTART): 队列空(JSQR==0)时硬件会清 JADSTART,
     * 不重新置位的话, 下一个 OC4REF 触发不会再启动转换 → JEOS 不再中断
     * → FOC 步进停摆(表现为电机停、灯不闪的"跑飞")。 */
    LL_ADC_INJ_StartConversion(ADC1);
    LL_ADC_INJ_StartConversion(ADC2);

    /** 双缓冲: OC4REF 比 UPDATE 早 1 tick → 本拍 ADC 用的是上一拍 pending */
    cb->active_sector  = cb->pending_sector;
    cb->pending_sector = s;
}

#endif /* DRV_CURR_FDBK_3SHUNT */

/** ============================================================
   ADC JEOC ISR

   2-shunt: ADC1→Iu=Ia, ADC2→Iv=Ib, Ic = -(Ia+Ib)
   3-shunt: 扇区映射 → 直读或推导 Ia/Ib/Ic
   校准模式: 直接累加 JDR 原始值, 不计算电流
   ============================================================ */
void drv_curr_fdbk_adc_isr(struct drv_curr_fdbk *cb)
{
    if (cb == NULL)
    {
        return;
    }

    uint16_t jdr1 = hal_adc1_inj_read_jdr1();
    uint16_t jdr2 = hal_adc2_inj_read_jdr1();

#if DRV_CURR_FDBK_TOPOLOGY == DRV_CURR_FDBK_3SHUNT
    /** 校准模式: 累加原始 ADC 值 (参考 MCSDK R3_2_HFCurrentsPolarization) */
    if (cb->calib_state != CURR_CALIB_STATE_IDLE)
    {
        if (cb->calib_state == CURR_CALIB_STATE_PHASE_AB)
        {
            /** Sector 5: ADC1=CH7(Iu)=Ia, ADC2=CH8(Iv)=Ib */
            cb->calib_sum_a += jdr1;
            cb->calib_sum_b += jdr2;
        }
        else  /**< CURR_CALIB_STATE_PHASE_C */
        {
            /** Sector 1: ADC2=CH9(Iw)=Ic */
            cb->calib_sum_c += jdr2;
        }
        cb->calib_count++;
        return;
    }
#endif

    int16_t ia, ib;

#if DRV_CURR_FDBK_TOPOLOGY == DRV_CURR_FDBK_2SHUNT

    ia = saturate_int16((int32_t)(cb->ia.offset) - (int32_t)jdr1);
    ib = saturate_int16((int32_t)(cb->ib.offset) - (int32_t)jdr2);

    cb->ia.adc_raw = jdr1;
    cb->ib.adc_raw = jdr2;
    cb->ic.adc_raw = 0u;

#else /* 3-shunt */

    uint16_t raw_a, raw_b, raw_c;

    raw_a = raw_b = raw_c = 0u;

    switch (cb->active_sector)
    {
    case CURR_SECTOR_4:
    case CURR_SECTOR_5:
        raw_a = jdr1;
        raw_b = jdr2;
        ia = saturate_int16((int32_t)(cb->ia.offset) - (int32_t)jdr1);
        ib = saturate_int16((int32_t)(cb->ib.offset) - (int32_t)jdr2);
        break;

    case CURR_SECTOR_6:
    case CURR_SECTOR_1:
        raw_b = jdr1;
        raw_c = jdr2;
        ib = saturate_int16((int32_t)(cb->ib.offset) - (int32_t)jdr1);
        ia = saturate_int16((int32_t)jdr2 - (int32_t)(cb->ic.offset) - (int32_t)ib);
        break;

    default:  /**< CURR_SECTOR_2, CURR_SECTOR_3 */
        raw_a = jdr1;
        raw_c = jdr2;
        ia = saturate_int16((int32_t)(cb->ia.offset) - (int32_t)jdr1);
        ib = saturate_int16((int32_t)jdr2 - (int32_t)(cb->ic.offset) - (int32_t)ia);
        break;
    }

    cb->ia.adc_raw = raw_a;
    cb->ib.adc_raw = raw_b;
    cb->ic.adc_raw = raw_c;

#endif

    cb->ia.i_s16 = ia;
    cb->ib.i_s16 = ib;
    cb->ic.i_s16 = saturate_int16(-(int32_t)ia - (int32_t)ib);

    /** 物理电流 (A): i_s16 × GAIN
     * i_s16 = (offset - raw) 已含扇区映射 + 零点校正, 是给 SMO 的正确电流
     * ADC 12-bit 左对齐 16-bit → GAIN 需 /16
     * Final_Gain = 3.3 / (4096 × 6 × 0.02) / 16 = 0.0004196 A/count */
#define CURR_GAIN_A  0.0004196f
    cb->ia.current_A = (float)(int32_t)cb->ia.i_s16 * CURR_GAIN_A;
    cb->ib.current_A = (float)(int32_t)cb->ib.i_s16 * CURR_GAIN_A;
    cb->ic.current_A = -(cb->ia.current_A + cb->ib.current_A);
}

/** ============================================================
   后台轮询: i_s16 → mA
   ============================================================ */
void drv_curr_fdbk_poll(struct drv_curr_fdbk *cb)
{
    if (cb == NULL)
    {
        return;
    }

    cb->ia.current_mA = ((int64_t)(cb->ia.i_s16) * cb->ia.gain_factor) >> 16;
    cb->ib.current_mA = ((int64_t)(cb->ib.i_s16) * cb->ib.gain_factor) >> 16;
    cb->ic.current_mA = ((int64_t)(cb->ic.i_s16) * cb->ic.gain_factor) >> 16;
}