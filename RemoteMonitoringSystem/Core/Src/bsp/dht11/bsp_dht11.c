#include "dht11\bsp_dht11.h"
#include "delay\bsp_delay.h"

/* Master reset DHT11 */
static void dht11_reset(void)
{
	DHT11_DQ_OUT(0);
	delay_ms(20);
	DHT11_DQ_OUT(1);
	delay_us(30);
}

/* Wait DHT11 response
		1:check error
		2:check success
*/
uint8_t dht11_check(void)
{
	uint8_t retry = 0;
	uint8_t rval = 0;
	
		while(DHT11_DQ_IN && retry < 100)
		{
			retry++;
			delay_us(1);
		}
		if(retry >= 100)
		{
			rval = 1;
		}
		else 
		{
			retry = 0;
			while(!DHT11_DQ_IN && retry < 100)
			{
				retry++;
				delay_us(1);
			}
			if(retry >= 100)	rval = 1;
		}
		return rval;
}

/* Read 1 bit data
	 return 1 / 0
*/
uint8_t dht11_read_bit(void)
{
	uint8_t retry = 0;
	
	while(DHT11_DQ_IN && retry < 100)			//Wait DHT11 pull down
	{
		retry++;
		delay_us(1);
	}
	
	retry = 0;
	
	while(!DHT11_DQ_IN && retry < 100)		//Wait DHT11 pull up
	{
		retry++;
		delay_us(1);
	}
	
	delay_us(40);
	
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

/**
 * @brief       从DHT11读取一次数据
 * @param       temp: 温度值(范围:0~50°)
 * @param       humi: 湿度值(范围:20%~90%)
 * @retval      0, 正常.
 *              1, 失败
 */
uint8_t dht11_read_data(uint8_t *temp, uint8_t *humi)
{
    uint8_t buf[5];
    uint8_t i;
    dht11_reset();

    if (dht11_check() == 0)
    {
        for (i = 0; i < 5; i++)     /* 读取40位数据 */
        {
            buf[i] = dht11_read_byte();
        }

        if ((buf[0] + buf[1] + buf[2] + buf[3]) == buf[4])
        {
            *humi = buf[0];
            *temp = buf[2];
        }
    }
    else
    {
        return 1;
    }
    
    return 0;
}

uint8_t dht11_init(void)
{
	dht11_reset();
	return dht11_check();
}
