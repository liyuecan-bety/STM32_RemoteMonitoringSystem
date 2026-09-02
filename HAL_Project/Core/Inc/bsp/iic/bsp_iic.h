#ifndef __BSP_IIC_H__
#define __BSP_IIC_H__

#include "main.h"

/*PORT define*/

#define IIC_SCL_GPIO_PORT							GPIOB
#define IIC_SCL_GPIO_PIN							GPIO_PIN_8
#define IIC_SCL_GPIO_CLK_ENABLE()			do{	__HAL_RCC_GPIOB_CLK_ENABLE();}while(0)

#define IIC_SDA_GPIO_PORT							GPIOB
#define IIC_SDA_GPIO_PIN							GPIO_PIN_9
#define	IIC_SDA_GPIO_CLK_ENABLE()			do{	__HAL_RCC_GPIOB_CLK_ENABLE();}while(0)

/*IO operation*/

#define IIC_SCL(x)										do{x ? \
																				 HAL_GPIO_WritePin(IIC_SCL_GPIO_PORT,IIC_SCL_GPIO_PIN,GPIO_PIN_SET):\
																				 HAL_GPIO_WritePin(IIC_SCL_GPIO_PORT,IIC_SCL_GPIO_PIN,GPIO_PIN_RESET);\
																			}while(0)

#define IIC_SDA(x)										do{x ? \
																				 HAL_GPIO_WritePin(IIC_SCL_GPIO_PORT,IIC_SDA_GPIO_PIN,GPIO_PIN_SET):\
																				 HAL_GPIO_WritePin(IIC_SCL_GPIO_PORT,IIC_SDA_GPIO_PIN,GPIO_PIN_RESET);\
																			}while(0)

#define IIC_READ_SDA									HAL_GPIO_ReadPin(IIC_SCL_GPIO_PORT,IIC_SCL_GPIO_PIN)																	

void iic_init(void);
static void iic_delay(void);
void iic_start(void);
void iic_stop(void);
uint8_t iic_wait_ack(void);
void iic_ack(void);
void iic_nack(void);
void iic_send_byte(uint8_t data);
uint8_t iic_read_byte(uint8_t ack);																		
#endif

																			

