#include "lsens\bsp_lsens.h"
#include "adc.h"

uint8_t lsens_get_val(void)
{
    uint32_t temp_val = 0;
    temp_val = adc3_get_result_average(ADC_CHANNEL_5, 10); /* 读取平均值 */
    temp_val /= 40;

    if (temp_val > 100)temp_val = 100;

    return (uint8_t)(100 - temp_val);
}


