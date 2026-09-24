/* Include file */
#include "usart\bsp_usart1.h"

/* 变量声明 */
uint8_t  g_usart_rx_buf[USART_REC_LEN];  /* RX buffer, max USART_REC_LEN bytes, last byte is terminator */
uint16_t g_usart_rx_sta;                 /* RX status register */
uint8_t g_rx_buffer[RXBUFFERSIZE];       /* HAL USART RX buffer */


//重新定向printf
int fputc(int ch,FILE *f)
{
	HAL_UART_Transmit(&huart1,(uint8_t*)&ch,1,100);
	return ch;
}

//中断回调函数
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart->Instance == USART1)
	{
		//0x0d->Receive \r
		//0x0a->Receive \n
		/*	The most significant bit represents the
				success falg of the reception
			*/
		if((g_usart_rx_sta & 0x8000) == 0)	 
		{
			/*	The second higest position is the
					acceptance of \r symbol
				*/
			if(g_usart_rx_sta & 0x4000)
			{
				//if g_rx_buffer[0]  is not \n
				if(g_rx_buffer[0] != 0x0a)
					g_usart_rx_sta = 0;
				//if g_rx_buffer[0]  is  \n
				else
					g_usart_rx_sta |= 0x8000;
			}
			/*	if the second highest position is not
					the acceptance of \r symbol
				*/
			else
			{
				//if g_rx_buffer[0] is \r
				if(g_rx_buffer[0] == 0x0d)
					g_usart_rx_sta |= 0x4000;
				//if g_rx_buffer[0] is not \r
				else
				{
					g_usart_rx_buf[g_usart_rx_sta &0x3fff] = g_rx_buffer[0];
					g_usart_rx_sta++;
					if(g_usart_rx_sta >= USART_REC_LEN - 1)
						g_usart_rx_sta = 0;
				}
			}
		}
		HAL_UART_Receive_IT(&huart1,(uint8_t*)g_rx_buffer,RXBUFFERSIZE);
	}

}

