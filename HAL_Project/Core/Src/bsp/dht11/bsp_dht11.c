#include "dht11\bsp_dht11.h"
#include "delay\bsp_delay.h"

/*	Master reset DHT11	*/
static void dht11_reset(void)
{
	DHT11_DQ_OUT(0);
	Delay_ms(20);
	DHT11_DQ_OUT(1);
	Delay_us(30);
}

/* Wait DHT11 response 
		1:check error
		0:check success
*/
uint8_t dht11_check(void)
{
	uint8_t retry = 0;
	uint8_t rval = 0;
	
	while((DHT11_DQ_IN) && (retry < 100))			//DHT11 will pull down 83us
	{
		retry++;
		Delay_us(1);
	}
	if(retry >= 100)
	{
		rval = 1;
	}
	else 
	{
		retry = 0;
		while((!DHT11_DQ_IN) && (retry < 100))	//DHT11 will pull up 87us
		{
			retry++;
			Delay_us(1);
		}
		if(retry >= 100)	rval = 0;
	}
	return rval;
}

/* Read 1 bit data
	 return 1 / 0
*/
uint8_t dht11_read_bit(void)
{
	uint8_t retry = 0;
	
	while((DHT11_DQ_IN) && (retry < 100))			//Wait DHT11 pull down
	{
		retry++;
		Delay_us(1);
	}
	
	retry = 0;
	
	while((!DHT11_DQ_IN) && (retry < 100))		//Wait DHT11 pull up
	{
		retry++;
		Delay_us(1);
	}
	
	Delay_us(40);
	
	if(DHT11_DQ_IN)
		return 1;
	else
		return 0;
}

/* Read 1 byte data */
static uint8_t dht11_read_byte(void)
{
	int i,data = 0;
	
	for(i = 0;i < 8;i++)					//Read 8-bit data in a loop
	{
		data <<= 1;									//Output the high-order data first,then shift left by on bit
		data |= dht11_read_bit();		//Read 1-bit data
	}	
	return data;
}