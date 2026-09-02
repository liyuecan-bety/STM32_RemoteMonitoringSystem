#ifndef __DWT_DELAY_H
#define __DWT_DELAY_H

#include "stm32f4xx_hal.h"

/* 初始化 DWT 周期计数器，main 里初始化后调用一次 */
void Delay_Init(void);

/* 微秒级延迟（阻塞） */
void Delay_us(uint32_t us);

/* 毫秒级延迟（阻塞），内部换算为 us */
void Delay_ms(uint32_t ms);

#endif /* __DWT_DELAY_H */


