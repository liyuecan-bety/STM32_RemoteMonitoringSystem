#ifndef __BSP_DHT11_H
#define __BSP_DHt11_H

#include "main.h"

#define DHT11_DQ_OUT(x)						do{x ? \
																			HAL_GPIO_WritePin(GPIOG,GPIO_PIN_9,GPIO_PIN_SET): \
																			HAL_GPIO_WritePin(GPIOG,GPIO_PIN_9,GPIO_PIN_RESET); \
																	}while(0)
#define DHT11_DQ_IN								HAL_GPIO_ReadPin(GPIOG,GPIO_PIN_9)
																		

uint8_t dht11_check(void);																						
uint8_t dht11_init(void);
uint8_t dht11_read_data(uint8_t *temp, uint8_t *humi);

#endif

																	