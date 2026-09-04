#include "delay\bsp_delay.h"

//对应主频1us对应的SysTick实际计数值
static uint32_t fac_us = 0;

//Initialize delay
void delay_init(void)
{
	if (SystemCoreClock == 0) {
		SystemCoreClockUpdate();  // 强制更新
  }
	//SystemCoreClock = 16000000，但是通过倍频之后会
	//变成168MHZ
	fac_us = SystemCoreClock / 1000000;	//fac_us = 168
}

void delay_us(uint32_t nus)
{
	uint32_t target_ticks;				//期望延时需要的总计数值
	uint32_t t_old = 0;						//旧值，即前一次的计数值
	uint32_t t_new = 0;						//新值，即当前的计数值
	uint32_t t_cnt = 0;						//计数器
	uint32_t reload = SysTick->LOAD;		//获取SysTick重装载值LOAD，一般为1ms对应的计数值
	
	target_ticks = nus * fac_us;
	
	t_old = SysTick->VAL;
	
	while(1)
	{
		t_new = SysTick->VAL;
		
		if(t_new != t_old)
		{
			if(t_new < t_old)
			{
				t_cnt += t_old - t_new;
			}
			else
			{
				t_cnt += reload + t_old - t_new;
			}
			t_old = t_new;
			if(t_cnt >= target_ticks)
				break;
		}
	}
}

void delay_ms(uint32_t nms)
{
	for(uint32_t i = 0;i < nms;i++)
	{
		delay_us(1000);
	}
}

