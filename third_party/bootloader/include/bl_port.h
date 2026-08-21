#ifndef BL_PORT_H
#define BL_PORT_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @file    bl_port.h
 * @brief   硬件端口接口 —— 换芯片时只需实现这一组函数
 *
 * 库不依赖任何厂商 HAL / 寄存器。所有硬件动作通过本结构注入。
 */

typedef struct {
    /**
     * @brief 擦除 [addr, addr+len) 覆盖的所有页
     * @param addr 页对齐的起始地址
     * @param len  长度（内部按页向上取整）
     * @return true 成功
     */
    bool (*flash_erase)(uint32_t addr, uint32_t len);

    /**
     * @brief 编程 len 字节（addr 与 len 须按 flash 编程宽度对齐，通常 8/16 字节）
     * @return true 成功
     */
    bool (*flash_program)(uint32_t addr, const uint8_t *buf, uint32_t len);

    /** @brief 软件复位（不返回） */
    void (*reset)(void);

    /** @brief 跳转 APP 前反初始化外设/关中断（可 NULL） */
    void (*deinit)(void);

    /** @brief 硬件触发进 IAP（如按键，无则 NULL 返回 false） */
    bool (*force_iap)(void);
} bl_port_t;

/** @brief 库持有的端口（bl_init 后有效），供内部模块访问 */
const bl_port_t *bl_port(void);

/**
 * @brief 跳转 APP（由端口实现：deinit + VTOR + MSP + 跳 Reset_Handler）
 * @note  需要设备头（stm32g4xx.h 等）提供 CMSIS，故放端口而非库内。
 */
void bl_jump_to_app(uint32_t app_base);

#endif /* BL_PORT_H */
