#include "delay\bsp_delay.h"
/* DWT 周期计数器是否已初始化（可避免重复初始化） */
static uint8_t dwt_initialized = 0;

/**
   * @brief  初始化 DWT 周期计数器
   * @note   需在 SystemClock_Config() 之后调用一次，
   *         确保 SystemCoreClock 已是实际主频。
*/
void Delay_Init(void)
{
    if (dwt_initialized)
    {
        return;
    }

    /* 使能内核调试对 DWT 的访问 */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    /* 清零并启动周期计数器 */
    DWT->CYCCNT = 0;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;

    dwt_initialized = 1;
}

/**
   * @brief  微秒级延迟
   * @param  us 延迟时间，单位微秒
   * @note   F407 默认主频 168MHz，1us = 168 个周期，
   *         用 SystemCoreClock 自动换算，换主频无需改代码。
 */
void Delay_us(uint32_t us)
{
    uint32_t start  = DWT->CYCCNT;
    uint32_t ticks  = us * (SystemCoreClock / 1000000UL);

    while ((DWT->CYCCNT - start) < ticks)
    {
          /* 空循环，等待周期计数达到目标 */
    }
}

/**
   * @brief  毫秒级延迟
   * @param  ms 延迟时间，单位毫秒
*/
void Delay_ms(uint32_t ms)
{
    Delay_us(ms * 1000UL);
}

