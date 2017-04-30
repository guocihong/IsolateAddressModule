#include "driver/adc/adc_drv.h"

void adc_init(void)
{    
	//打开AD电源
    //设置AD转换速率：540个时钟周期转换一次
    ADC_CONTR = ADC_POWER_ENABLE | ADC_SPEED_540;
    
	//关闭ADC中断
	EADC = 0;
	
    //设置AD转换结果存储格式
    AUXR1 &= 0xFB;//10位AD转换结果高8位存储在ADC_RES,低2位存储在ADC_RESL中
}
