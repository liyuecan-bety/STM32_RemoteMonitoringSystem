#ifndef __BSP_DHT11_H__
#define __BSP_DHT11_H__

#include "main.h"

#define DHT11_DQ_OUT(x)						do{x ? \
																			HAL_GPIO_WritePin(GPIOF,GPIO_PIN_9,GPIO_PIN_SET): \
																			HAL_GPIO_WritePin(GPIOF,GPIO_PIN_9,GPIO_PIN_RESET); \
																	}while(0)
#define DHT11_DQ_IN								HAL_GPIO_ReadPin(GPIOF,GPIO_PIN_9)
																		
static void dht11_reset(void);
uint8_t dht11_check(void);						
uint8_t dht11_read_bit(void);																	
static uint8_t dht11_read_byte(void);																																																		
#endif



	