#include "driver/adc/adc_drv.h"

void adc_init(void)
{    
	//��AD��Դ
    //����ADת�����ʣ�540��ʱ������ת��һ��
    ADC_CONTR = ADC_POWER_ENABLE | ADC_SPEED_540;
    
	//�ر�ADC�ж�
	EADC = 0;
	
    //����ADת������洢��ʽ
    AUXR1 &= 0xFB;//10λADת�������8λ�洢��ADC_RES,��2λ�洢��ADC_RESL��
}
