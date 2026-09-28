/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcd\lcd.h"
#include "dht11\bsp_dht11.h"
#include "lsens\bsp_lsens.h"
#include "LedKey\bsp_led_key.h"
#include "delay\bsp_delay.h"
#include <stdio.h>
#include "usart.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* Data shared between the sensor tasks and the LCD task. */
typedef struct
{
    uint8_t light;        /* light intensity 0~100 */
    uint8_t temperature;  /* temperature */
    uint8_t humidity;     /* humidity */
    uint8_t dht11_ok;     /* 1 = DHT11 ok, 0 = error */
} monitor_data_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
static monitor_data_t g_monitor = {0, 0, 0, 0};   /* shared sensor data */
static osMutexId_t     g_monitor_mutex;           /* mutex protecting g_monitor */

/* Task handles and attributes */
osThreadId_t lightTaskHandle;
const osThreadAttr_t lightTask_attributes = {
  .name = "lightTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
osThreadId_t dht11TaskHandle;
const osThreadAttr_t dht11Task_attributes = {
  .name = "dht11Task",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
osThreadId_t lcdTaskHandle;
const osThreadAttr_t lcdTask_attributes = {
  .name = "lcdTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
osThreadId_t ledTaskHandle;
const osThreadAttr_t ledTask_attributes = {
  .name = "ledTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
osThreadId_t keyTaskHandle;
const osThreadAttr_t keyTask_attributes = {
	.name = "keyTask",
	.stack_size = 128 * 4,
	.priority = (osPriority_t) osPriorityNormal,
};
osThreadId_t uartTxTaskHandle;
const osThreadAttr_t uartTxTask_attributes = {
  .name = "uartTxTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* USER CODE END Variables */



/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void StartLightTask(void *argument);
void StartDHT11Task(void *argument);
void StartLcdTask(void *argument);
void StartLedTask(void *argument);
void StartKeyTask(void *argument);
void StartUartTxTask(void *argument);
/* USER CODE END FunctionPrototypes */


void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  /* Create the mutex that protects the shared sensor data. */
  g_monitor_mutex = osMutexNew(NULL);
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */

  /* USER CODE BEGIN RTOS_THREADS */
  lightTaskHandle = osThreadNew(StartLightTask, NULL, &lightTask_attributes);
  dht11TaskHandle = osThreadNew(StartDHT11Task, NULL, &dht11Task_attributes);
  lcdTaskHandle   = osThreadNew(StartLcdTask,   NULL, &lcdTask_attributes);
  ledTaskHandle   = osThreadNew(StartLedTask,   NULL, &ledTask_attributes);
	keyTaskHandle		= osThreadNew(StartKeyTask,		NULL,	&keyTask_attributes);
	uartTxTaskHandle = osThreadNew(StartUartTxTask,NULL,&uartTxTask_attributes);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/**
  * @brief  Light sensor task: reads the ADC and stores the light value.
  * @param  argument: not used
  * @retval None
  */
void StartLightTask(void *argument)
{
  for(;;)
  {
    uint8_t light = lsens_get_val();   /* ~50ms: 10 ADC samples */

    osMutexAcquire(g_monitor_mutex, osWaitForever);
    g_monitor.light = light;
    osMutexRelease(g_monitor_mutex);

    osDelay(200);
  }
}

/**
  * @brief  DHT11 task: reads temperature and humidity.
  * @note   The DHT11 driver is interrupt-driven: the EXTI peripheral captures
  *         the single-bus edges and time-stamps them with the DWT counter, and
  *         this task blocks on a semaphore until the read completes. No busy-wait
  *         and no critical section are needed.
  * @param  argument: not used
  * @retval None
  */
void StartDHT11Task(void *argument)
{
  uint8_t temp, humi, ok;

  dht11_init();   /* set up EXTI + completion semaphore (once) */

  for(;;)
  {
    ok = (dht11_read_data(&temp, &humi) == 0);   /* blocks on semaphore */

    osMutexAcquire(g_monitor_mutex, osWaitForever);
    g_monitor.dht11_ok = ok;
    if (ok)
    {
      g_monitor.temperature = temp;
      g_monitor.humidity    = humi;
    }
    osMutexRelease(g_monitor_mutex);

    osDelay(2000);   /* DHT11 sampling period must be >= 2 s */
  }
}

/**
  * @brief  LCD task: refreshes the display with the latest sensor data.
  * @param  argument: not used
  * @retval None
  */
void StartLcdTask(void *argument)
{
  monitor_data_t data;
  uint8_t last_ok = 0xFF;   /* sentinel: forces the first status draw */

  for(;;)
  {
    osMutexAcquire(g_monitor_mutex, osWaitForever);
    data = g_monitor;
    osMutexRelease(g_monitor_mutex);

    lcd_show_num(70, 150, data.temperature, 2, 16, BLUE);
    lcd_show_num(70, 170, data.humidity,    2, 16, BLUE);
    lcd_show_xnum(110, 110, data.light, 3, 16, 0, BLUE);

    if (last_ok != data.dht11_ok)   /* redraw status only on change */
    {
      last_ok = data.dht11_ok;
      lcd_fill(30, 130, 30 + 10 * 8 - 1, 130 + 16 - 1, WHITE);
      if (data.dht11_ok)
      {
        lcd_show_string(30, 130, 200, 16, 16, "DHT11 OK", RED);
      }
      else
      {
        lcd_show_string(30, 130, 200, 16, 16, "DHT11 ERROR", RED);
      }
    }

    osDelay(200);
  }
}

/**
  * @brief  LED task: toggles LED0 every 200ms and LED1 every 1s.
  * @param  argument: not used
  * @retval None
  */
void StartLedTask(void *argument)
{
  uint32_t cnt = 0;

  for(;;)
  {
    cnt++;
    LED0_TOGGLE;             /* every 200ms -> ~2.5Hz */
//    if (cnt % 5 == 0)
//    {
//      LED1_TOGGLE;           /* every 1s -> 1Hz */
//    }
    osDelay(200);
	}
}

//KEY Task
void StartKeyTask(void *argument)
{
	uint8_t last = 1;
		for(;;)
		{
			uint8_t now = KEY0;
			if(last==1 && now == 0)
				LED1_TOGGLE;
			last = now;
			osDelay(10);
		}
}

//uartTx Task
void StartUartTxTask(void *argument)
{
	char buf[64];
	monitor_data_t d;
	int n;
	
	for(;;)
	{
		osMutexAcquire(g_monitor_mutex,osWaitForever);
		d = g_monitor;
		osMutexRelease(g_monitor_mutex);
		
		n = snprintf(buf,sizeof(buf),"T=%d,H=%d,L=%d,S=%d\r\n"
			,d.temperature,d.humidity,d.light,d.dht11_ok);
		HAL_UART_Transmit(&huart2,(uint8_t*)buf,(uint16_t)n,100);
		osDelay(1000);
		
	}

}
/* USER CODE END Application */

