#ifndef __BSP_DHT11_H
#define __BSP_DHT11_H

#include "main.h"

#define DHT11_DQ_PORT       GPIOG
#define DHT11_DQ_PIN        GPIO_PIN_9

void    dht11_init(void);
uint8_t dht11_read_data(uint8_t *temp, uint8_t *humi);

#endif
