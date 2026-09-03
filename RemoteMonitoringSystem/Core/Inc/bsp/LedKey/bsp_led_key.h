#ifndef __BSP_LED_KEY_H
#define __BSP_LED_KEY_H

#include "main.h"

#define LED0(x)								do{x ? \
																	HAL_GPIO_WritePin(GPIOF,GPIO_PIN_9,GPIO_PIN_SET):\
																	HAL_GPIO_WritePin(GPIOF,GPIO_PIN_9,GPIO_PIN_RESET);\
															}while(0)
#define LED1(x)								do{x ? \
																	HAL_GPIO_WritePin(GPIOF,GPIO_PIN_10,GPIO_PIN_SET):\
																	HAL_GPIO_WritePin(GPIOF,GPIO_PIN_10,GPIO_PIN_RESET);\
															}while(0)
#define LED0_TOGGLE						HAL_GPIO_TogglePin(GPIOF,GPIO_PIN_9)
#define LED1_TOGGLE						HAL_GPIO_TogglePin(GPIOF,GPIO_PIN_10)															

#define KEY0									HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_4)
#define KEY1									HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_3)
#define KEY2									HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_2)
#define KEY_WKUP							HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0)

#define KEY0_PRES							1		
#define KEY1_PRES							2	
#define KEY2_PRES							3	
#define KEY_WKUP_PRES					4																
#endif


