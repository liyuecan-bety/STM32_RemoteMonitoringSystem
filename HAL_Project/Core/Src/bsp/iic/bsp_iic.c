#include "iic\bsp_iic.h"
#include "delay\bsp_delay.h"


/*Initialize IIC*/
void iic_init(void)
{
	//define GPIO struct
	GPIO_InitTypeDef GPIO_InitStruct;
	
	//Enable GPIO Clock
	IIC_SCL_GPIO_CLK_ENABLE();
	IIC_SDA_GPIO_CLK_ENABLE();
	
	//Initialize GPIOB
	
	GPIO_InitStruct.Pin = IIC_SCL_GPIO_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(IIC_SCL_GPIO_PORT,&GPIO_InitStruct);
	
	GPIO_InitStruct.Pin = IIC_SDA_GPIO_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	HAL_GPIO_Init(IIC_SDA_GPIO_PORT,&GPIO_InitStruct);
	
	//Stop all devices on the bus
	iic_stop();
}

/*IIC delay function to control the read and write speed of IIC*/
static void iic_delay(void)
{
	Delay_us(2);
}

/*generate IIC strat signal*/
void iic_start(void)
{
	IIC_SDA(1);
	IIC_SCL(1);
	iic_delay();
	IIC_SDA(0);			/*START signal:when SCL is high,SDA from high to low */
	iic_delay();
	IIC_SCL(0);			/*Grab the IIC bus,ready to transmit or receive data */
	iic_delay();
}

/*Generate IIC stop signal*/
void iic_stop(void)
{
	IIC_SDA(0);			/*STOP signal:when SCL is high,SDA from low to high */
	iic_delay();
	IIC_SCL(1);			
	iic_delay();
	IIC_SDA(1);
	iic_delay();
}

/*Wait for the response signal arrive
	1,receive response signal failure
	0,receive response signal success
*/
uint8_t iic_wait_ack(void)
{
	uint8_t waittime = 0;
	uint8_t rack = 0;
	
	IIC_SDA(1);		/* master device releases SDA(now )*/
	iic_delay();
	IIC_SCL(1);		/* SCL = 1,now slave device can return ACK */
	iic_delay();
	
	while(IIC_READ_SDA)
	{
		waittime++;
		if(waittime > 250)
		{
			iic_stop();
			rack = 1;
			break;
		}
	}
	IIC_SCL(0);		/* SCL = 0, end ACK check */
	iic_delay();
	return rack;
}

/* Generate ACK response signal
	ACK：
	发送方在第9个时钟释放SDA（输出高电平，不驱动）

	接收方主动拉低SDA（产生ACK）

	如果接收方不拉低，上拉电阻保持SDA为高（NACK）
*/
void iic_ack(void)
{
	IIC_SDA(0);		/*SCL 0 -> 1 and SDA = 0 response */
	iic_delay();
	IIC_SCL(1);		/* Generate a clock */
	iic_delay();
	IIC_SCL(0);
	iic_delay();
	IIC_SDA(1);		/* master releases SDA */
	iic_delay();
}

/* Does not generate ACK response signal */
void iic_nack(void)
{
	IIC_SDA(1);		/*When SCL 0 -> 1 ,SDA = 1 , does not response */
	iic_delay();
	IIC_SCL(1);		/* Generate a clock */
	iic_delay();
	IIC_SCL(0);
	iic_delay();
}

/* IIC sends a byte */
void iic_send_byte(uint8_t data)
{
	uint8_t t;
	for(t = 0;t < 8;t++)
	{
		IIC_SDA((data & 0x80) >> 7);
		iic_delay();
		IIC_SCL(1);
		iic_delay();
		IIC_SCL(0);
		data <<= 1;
	}
	IIC_SDA(1);
}

/* IIC read a byte */
uint8_t iic_read_byte(uint8_t ack)
{
	uint8_t i,receive = 0;
	for(i = 0;i < 8;i++)
	{
		receive <<= 1;		/* The higher bits are output first,so the received data needs to be shifted to left */
		IIC_SCL(1);
		iic_delay();
		if(IIC_READ_SDA)
			receive++;
		IIC_SCL(0);
		iic_delay();
	}
	if(!ack)
		iic_nack();
	else
		iic_ack();
	return receive;
}

