/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"
#include "fsmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "led\bsp_led.h"
#include "lcd\lcd.h"
#include <stdio.h>
#include "key\bsp_key.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ADC_SAMPLE_COUNT 			10			//sample number
#define ADC_CHANNEL_COUNT 		6				//channel number
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint16_t adc_dma_buffer [ADC_SAMPLE_COUNT][ADC_CHANNEL_COUNT];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	//every channel sum、average、voltage
	uint32_t ch_sum[ADC_CHANNEL_COUNT] = {0};
	uint32_t ch_avg[ADC_CHANNEL_COUNT] = {0};
	float ch_voltage[ADC_CHANNEL_COUNT] = {0};
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_FSMC_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
	//LCD Initialize
	lcd_init();							
	
	//LCD display title
		lcd_show_string(30,  50, 200, 16, 16, "STM32", RED);
    lcd_show_string(30,  70, 200, 16, 16, "ADC 6CH DMA TEST", RED);
    lcd_show_string(30,  90, 200, 16, 16, "ATOM@ALIENTEK", RED);

    lcd_show_string(30, 110, 200, 12, 12, "ADC1_CH0_VAL:", BLUE);
    lcd_show_string(30, 122, 200, 12, 12, "ADC1_CH0_VOL:0.000V", BLUE); /* 先在固定位置显示小数点 */
    
    lcd_show_string(30, 140, 200, 12, 12, "ADC1_CH1_VAL:", BLUE);
    lcd_show_string(30, 152, 200, 12, 12, "ADC1_CH1_VOL:0.000V", BLUE); /* 先在固定位置显示小数点 */

    lcd_show_string(30, 170, 200, 12, 12, "ADC1_CH2_VAL:", BLUE);
    lcd_show_string(30, 182, 200, 12, 12, "ADC1_CH2_VOL:0.000V", BLUE); /* 先在固定位置显示小数点 */

    lcd_show_string(30, 200, 200, 12, 12, "ADC1_CH3_VAL:", BLUE);
    lcd_show_string(30, 212, 200, 12, 12, "ADC1_CH3_VOL:0.000V", BLUE); /* 先在固定位置显示小数点 */

    lcd_show_string(30, 230, 200, 12, 12, "ADC1_CH4_VAL:", BLUE);
    lcd_show_string(30, 242, 200, 12, 12, "ADC1_CH4_VOL:0.000V", BLUE); /* 先在固定位置显示小数点 */

    lcd_show_string(30, 260, 200, 12, 12, "ADC1_CH5_VAL:", BLUE);
    lcd_show_string(30, 272, 200, 12, 12, "ADC1_CH5_VOL:0.000V", BLUE); /* 先在固定位置显示小数点 */
	
	
	//Start ADC + DMA CIRCULAR MODE
	HAL_ADC_Start_DMA(&hadc1,(uint32_t*)adc_dma_buffer,ADC_SAMPLE_COUNT * ADC_CHANNEL_COUNT);
	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */		
		// ========== 1. 先清零累加器 ==========
    for (int ch = 0; ch < ADC_CHANNEL_COUNT; ch++) {
        ch_sum[ch] = 0;  // ✅ 关键：每次循环必须清零
    }
		
		//get every channel's sample value 
		for(int sample = 0;sample < ADC_SAMPLE_COUNT;sample++){
			for(int ch = 0;ch < ADC_CHANNEL_COUNT;ch ++){
				ch_sum[ch] += adc_dma_buffer[sample][ch];
			}
		}
		
		//get every channel's average and voltage : sum / ADC_SAMPLE_COUNT
		for (int ch = 0; ch < ADC_CHANNEL_COUNT; ch++) {
        ch_avg[ch] = ch_sum[ch] / ADC_SAMPLE_COUNT;  // ✅ 除以采样次数 10
        ch_voltage[ch] = (float)ch_avg[ch] * 3.3f / 4096.0f;
    }
		
		//Show ADC's value
		// ========== 4. 显示 ADC 值和电压 ==========
    for (int ch = 0; ch < ADC_CHANNEL_COUNT; ch++) {
        // 显示 ADC 原始值
        lcd_show_xnum(108, 110 + (ch * 30), ch_avg[ch], 4, 12, 0, BLUE);
        
        // 显示电压值
        uint16_t vol_int = (uint16_t)ch_voltage[ch];           // 整数部分
        uint16_t vol_dec = (uint16_t)((ch_voltage[ch] - vol_int) * 1000);  // 小数部分（mV）
        
        lcd_show_xnum(108, 122 + (ch * 30), vol_int, 1, 12, 0, BLUE);
        lcd_show_xnum(120, 122 + (ch * 30), vol_dec, 3, 12, 0x80, BLUE);
    }
			
		
		LED0_TOGGLE;
		HAL_Delay(500);
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
