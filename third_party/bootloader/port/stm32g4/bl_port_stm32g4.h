#ifndef BL_PORT_STM32G4_H
#define BL_PORT_STM32G4_H

#include "bl_port.h"

/**
 * @file    bl_port_stm32g4.h
 * @brief   STM32G4 参考端口
 *
 * 用法：
 *   bl_port_stm32g4_init();          //// 解锁 flash 等
 *   bl_init(bl_port_stm32g4_get());  //// 绑定端口
 */

/** 端口初始化（unlock flash），在 bl_init 前调用 */
void bl_port_stm32g4_init(void);

/** 返回 STM32G4 端口 */
const bl_port_t *bl_port_stm32g4_get(void);

#endif /* BL_PORT_STM32G4_H */