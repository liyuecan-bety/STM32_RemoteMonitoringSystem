#include "dht11\bsp_dht11.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/*
 * DHT11 single-bus driver, interrupt-driven version.
 *
 * Timing is captured with the EXTI peripheral (both edges on the data pin)
 * and time-stamped with the DWT cycle counter, so there is no microsecond
 * busy-wait and no need to suspend the scheduler. The read task simply
 * blocks on a binary semaphore that the EXTI ISR releases when the 40-bit
 * response has been fully captured.
 */

#define DHT11_BIT_COUNT     40
#define DHT11_MAX_EDGES     96          /* 2 response + 40*2 data + 2 trailing */
#define DHT11_EDGE_TARGET   83          /* edges needed to decode all 40 bits */

static SemaphoreHandle_t dht11_sem;     /* ISR -> task completion signal */
static volatile uint32_t  g_edges[DHT11_MAX_EDGES];   /* DWT timestamps */
static volatile uint8_t   g_edge_cnt;
static volatile uint8_t   g_recording;  /* 1 while a read is in progress */
static uint32_t           g_cycles_per_us;   /* e.g. 168 for 168 MHz */

/* --- pin mode helpers ------------------------------------------------ */
static void dht11_pin_output(void)
{
    GPIO_InitTypeDef s = {0};
    s.Pin   = DHT11_DQ_PIN;
    s.Mode  = GPIO_MODE_OUTPUT_OD;     /* open-drain: can pull low, floats high via pull-up */
    s.Pull  = GPIO_PULLUP;
    s.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_DQ_PORT, &s);
}

static void dht11_pin_input(void)
{
    GPIO_InitTypeDef s = {0};
    s.Pin   = DHT11_DQ_PIN;
    s.Mode  = GPIO_MODE_IT_RISING_FALLING;   /* EXTI on both edges */
    s.Pull  = GPIO_PULLUP;
    s.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_DQ_PORT, &s);
    __HAL_GPIO_EXTI_CLEAR_IT(DHT11_DQ_PIN);  /* drop any stale pending flag */
}

/* --- init ------------------------------------------------------------ */
void dht11_init(void)
{
    g_cycles_per_us = SystemCoreClock / 1000000;

    /* Enable the DWT cycle counter used for edge time-stamping (idempotent). */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;

    dht11_sem = xSemaphoreCreateBinary();
		if (dht11_sem == NULL) {
				Error_Handler();   // »ò return ´íÎóÂë
		}
    dht11_pin_output();

    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

/* --- EXTI callback (runs in interrupt context) ------------------------ */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (GPIO_Pin != DHT11_DQ_PIN || g_recording == 0) {
        return;
    }

    if (g_edge_cnt < DHT11_MAX_EDGES) {
        g_edges[g_edge_cnt++] = DWT->CYCCNT;
    }

    /* All 40 bits are available once 83 edges have been captured. */
    if (g_edge_cnt >= DHT11_EDGE_TARGET) {
        g_recording = 0;
        if (dht11_sem != NULL) {
            xSemaphoreGiveFromISR(dht11_sem, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

/* --- decode ----------------------------------------------------------- */
static uint8_t dht11_decode(uint8_t *temp, uint8_t *humi)
{
    uint8_t  buf[5] = {0};// save data
    uint32_t threshold = 40U * g_cycles_per_us;   /* 40us: 0->26-28us, 1->70us */
    uint8_t  i;

    if (g_edge_cnt < DHT11_EDGE_TARGET) {
        return 1;   /* not enough edges captured */
    }

    /*
     * Edge layout after the start signal:
     *   [0] F  response low      [1] R  response high
     *   [2+2k] F  bit k low      [3+2k] R  bit k high
     * Bit k is decided by the high-pulse width: edges[4+2k] - edges[3+2k].
     */
    for (i = 0; i < DHT11_BIT_COUNT; i++) {
        uint32_t high = g_edges[4 + 2 * i] - g_edges[3 + 2 * i];
        buf[i / 8] <<= 1;
        if (high > threshold) {
            buf[i / 8] |= 1;
        }
    }

    if ((uint8_t)(buf[0] + buf[1] + buf[2] + buf[3]) != buf[4]) {
        return 1;   /* checksum error */
    }

    *humi = buf[0];
    *temp = buf[2];
    return 0;
}

/* --- read ------------------------------------------------------------- */
uint8_t dht11_read_data(uint8_t *temp, uint8_t *humi)
{
    uint8_t result = 1;

    /* Drain any stale semaphore token from a previous read. */
    xSemaphoreTake(dht11_sem, 0);

    /* 1. Start signal: pull low >= 18ms (task sleeps, other tasks run). */
    dht11_pin_output();
    HAL_GPIO_WritePin(DHT11_DQ_PORT, DHT11_DQ_PIN, GPIO_PIN_RESET);
    vTaskDelay(pdMS_TO_TICKS(20));

    /* 2. Release the line and switch to input + EXTI to capture the response. */
    HAL_GPIO_WritePin(DHT11_DQ_PORT, DHT11_DQ_PIN, GPIO_PIN_SET);   /* float high via pull-up */
    g_edge_cnt = 0;
		g_recording = 0;        // ¡û ÏÔÊ½½ûÖ¹¼ÇÂ¼
    dht11_pin_input();
    g_recording = 1;

    /* 3. Block until the ISR finishes capturing (no busy-wait). */
    if (xSemaphoreTake(dht11_sem, pdMS_TO_TICKS(10)) == pdTRUE) {
        result = dht11_decode(temp, humi);
    }

    /* 4. Back to idle output state. */
    g_recording = 0;
    dht11_pin_output();
    HAL_GPIO_WritePin(DHT11_DQ_PORT, DHT11_DQ_PIN, GPIO_PIN_SET);

    return result;
}
