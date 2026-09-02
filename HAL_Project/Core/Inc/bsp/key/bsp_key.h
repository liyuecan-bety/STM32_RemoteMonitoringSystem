#ifndef __BSP_KEY_H__
#define __BSP_KEY_H__

#include "main.h"

#define KEY_WKUP					HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0)
#define KEY0							HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_4)
#define KEY1							HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_3)
#define KEY2							HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_2)

		
#define KEY0_PRES								1
#define KEY1_PRES								2
#define KEY2_PRES								3
#define KEY_WKUP_PRES						4

uint8_t key_scan(uint8_t mode);
#endif


