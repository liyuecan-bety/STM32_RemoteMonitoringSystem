#ifndef __BSP_LED_H__
#define __BSP_LED_H__

/*include*/
#include "main.h"

/*define*/
#define LED0(x) 				do{x ? \
														HAL_GPIO_WritePin(GPIOF,GPIO_PIN_9,GPIO_PIN_SET):\
														HAL_GPIO_WritePin(GPIOF,GPIO_PIN_9,GPIO_PIN_RESET);\
													}while(0)
#define LED1(x) 				do{x ? \
														HAL_GPIO_WritePin(GPIOF,GPIO_PIN_10,GPIO_PIN_SET):\
														HAL_GPIO_WritePin(GPIOF,GPIO_PIN_10,GPIO_PIN_RESET);\
													}while(0)

#define LED0_TOGGLE			do{HAL_GPIO_TogglePin(GPIOF,GPIO_PIN_9);}while(0)
#define LED1_TOGGLE			do{HAL_GPIO_TogglePin(GPIOF,GPIO_PIN_10);}while(0)
#endif


