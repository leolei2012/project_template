#ifndef HAL_USART2_H
#define HAL_USART2_H

#include "platform.h"

void hal_usart2_init(void);

/** ——— DMA 发送 ——— */
uint8_t hal_usart2_dma_tx_start(const uint8_t *buf, uint16_t len);
void hal_usart2_dma_tx_isr(void);  /**< DMA1_CH2 中断入口 */

/** ——— 中断接收（逐字节） ——— */
void    hal_usart2_enable_it_rxne(void);
void    hal_usart2_disable_it_rxne(void);
uint8_t hal_usart2_read_byte(void);

#endif /* HAL_USART2_H */