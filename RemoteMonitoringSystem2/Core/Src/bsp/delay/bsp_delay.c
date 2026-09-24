#include "delay\bsp_delay.h"

/* CPU cycles per 1us, e.g. 168MHz -> 168 */
static uint32_t fac_us = 0;

/* Initialize the DWT cycle counter (Cortex-M4 core peripheral, uses no extra timer). */
void delay_init(void)
{
    if (SystemCoreClock == 0) {
        SystemCoreClockUpdate();     /* force clock update */
    }
    fac_us = SystemCoreClock / 1000000;   /* cycles per microsecond */

    /* Enable the DWT CYCCNT counter */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;   /* enable trace unit */
    DWT->CYCCNT = 0;                                  /* reset cycle counter */
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;             /* enable cycle counter */
}

/* Microsecond busy-wait using the DWT cycle counter.
 * Independent of SysTick, so it works even before the FreeRTOS scheduler
 * starts and is not affected by the FreeRTOS tick configuration. */
void delay_us(uint32_t nus)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = nus * fac_us;    /* cycle count to wait for */

    while ((DWT->CYCCNT - start) < ticks) {
        ;
    }
}

void delay_ms(uint32_t nms)
{
    for (uint32_t i = 0; i < nms; i++) {
        delay_us(1000);
    }
}
