#ifndef __BSP_USART1_H
#define __BSP_USART1_H

#include "main.h"
#include "usart.h"
#include <stdio.h>

#define USART_REC_LEN   200                     /* Receive buffer length: 200 bytes */
#define RXBUFFERSIZE    1                       /* Receive buffer size: 1 byte per interrupt */

extern uint8_t  g_usart_rx_buf[USART_REC_LEN];  /* RX buffer, max USART_REC_LEN bytes, last byte is terminator */
extern uint16_t g_usart_rx_sta;                 /* RX status register */
extern uint8_t g_rx_buffer[RXBUFFERSIZE];       /* HAL USART RX buffer */

#endif

