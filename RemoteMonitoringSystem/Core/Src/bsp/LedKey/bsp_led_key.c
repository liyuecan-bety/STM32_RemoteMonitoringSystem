#include "LedKey\bsp_led_key.h"


//按键检测，mode = 1,可以连按；mode = 0,不能连按
uint8_t key_scan(uint8_t mode)
{
	static uint8_t key_up = 1; /* 按键松开标志 */
	uint8_t keyval;						/* 返回的按键值 */
	
	if(mode) key_up = 1;
	
	if(key_up && (KEY0 == 0 || KEY1 == 0 || KEY2 == 0 || KEY_WKUP == 1))
	{
		HAL_Delay(10);
		key_up = 0;
		
		if(KEY0 == 0) keyval = KEY0_PRES;
		if(KEY1 == 0) keyval = KEY1_PRES;
		if(KEY2 == 0) keyval = KEY2_PRES;
		if(KEY_WKUP == 1) keyval = KEY_WKUP_PRES;
	
	}
	
	else if(KEY0 == 1 && KEY1 == 1 && KEY2 == 1 && KEY_WKUP == 0)
	{
		key_up = 1;
	}
	
	return keyval;
	
}

