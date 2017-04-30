#include "driver/timer/timer_drv.h"
#include "driver/wdt/wdt_drv.h"
#include "driver/adc/adc_drv.h"

/* UART2 */
extern xdata  Byte         recv2_state;                 // receive state
extern xdata  Byte         recv2_timer;                 // receive time-out, �����ֽڼ䳬ʱ�ж�

/* System */
extern data   Uint16       gl_ack_tick;     	        //��λ��485��Ӧ����ʱ��ʱ tick
extern data   Byte         watch_dog_tick;

/* AD sample */
extern  data  Byte         ad_index;                    //���ڲ�����ͨ���ţ���Χ0-1
extern  data  sAD_Sample   ad_sample;                   //���浱ǰ����ֵ
extern  data  Uint16       ad_alarm_tick[2];            //��ͨ��������ʱtick

void timer0_init(void)   // 5ms@22.1184MHz
{    
	//watch_dog_tick = 0xFF;
	
    // ��ʱ��0��ʼ��
	AUXR &= 0x7F;		 // ����Ϊ12Tģʽ
	TMOD &= 0xF0;		 // ����Ϊ����ģʽ1
	TMOD |= 0x01;
	TL0 = 0x00;		     // ���ö�ʱ��ֵ
	TH0 = 0xDC;		     // ���ö�ʱ��ֵ
	//TH0 = 0x28;		     // ���ö�ʱ��ֵ
	TF0 = 0;		     // ���TF0��־
    ET0 = 1;             // ʹ��T0�ж�����λ
    IPH |= (1 << 1);
    PT0 = 0;             // �����ж����ȼ�Ϊ���ȼ�2
	TR0 = 1;		     // ��ʱ��0��ʼ��ʱ
	
	// ����ADת��
	ADC_CONTR &= 0xF8;   //ѡ��P10��ΪADת��
    _nop_();
    _nop_();
    _nop_();
    _nop_();
	ADC_CONTR |= ADC_START;
}

void timer0_isr(void) interrupt TF0_VECTOR using 0
{	      
	Byte i;
	
    // ��װ��ֵ
    TR0 = 0;
	TL0 = 0x00;		                              // ���ö�ʱ��ֵ
	TH0 = 0xDC;		                              // ���ö�ʱ��ֵ
	//TH0 = 0x28;		                              // ���ö�ʱ��ֵ
	TR0 = 1;		                              // ��ʱ��0��ʼ��ʱ
        
	// ADת�����,��ADC_FLAGת����ɱ�־����
    ADC_CONTR &= ~ADC_FLAG;

	// ��ADֵ
	if (ad_sample.valid == FALSE) {
		// ԭ�����Ѿ�������, ����д��������
		ad_sample.val   = ADC_RES;                // ����8λ
		ad_sample.val   = ad_sample.val << 2;
		ad_sample.val   += (ADC_RESL & 0x03);     // �õ�10bit����ֵ
		ad_sample.index = ad_index;
		ad_sample.valid = TRUE;
			        
		// ������һͨ������
		if (ad_index == 1) {
			ad_index = 0;
		} else {
			ad_index++;
		}

		// ѡ��ģ������
		if (ad_index == 0) {//ѡ��P10��ΪAD����
			ADC_CONTR &= 0xF8;
		} else if (ad_index == 1) {//ѡ��P11��ΪAD����
			ADC_CONTR &= 0xF8;
			ADC_CONTR |= 0x01;
		}
		
        _nop_();
        _nop_();
        _nop_();
        _nop_();
        _nop_();
        _nop_();
        _nop_();
        _nop_();
   		ADC_CONTR |= ADC_POWER_ENABLE | ADC_SPEED_540; // ����ת��
        _nop_();
        _nop_();
        _nop_();
        _nop_();
        ADC_CONTR |=  ADC_START;
	}
	
	if (gl_ack_tick > 0) {
		gl_ack_tick--;                            //Ӧ����ʱ��ʱ
	}
	
	for (i = 0; i < 2; i++) {
		if (ad_alarm_tick[i] > 0) {
			ad_alarm_tick[i]--;
		}
	}
		
	/*
	if (watch_dog_tick > 0) {
		watch_dog_tick--;
		
		//ι��
        Wdt_refresh();
	}
	*/
	
	// UART2�ֽ�֮����ճ�ʱ
	if (recv2_state != FSA_INIT) { 
		//�ǳ�ʼ״̬����Ҫ����Ƿ�ʱ
		if (recv2_timer > 0) {
			recv2_timer--;
		}
		
		if (recv2_timer == 0) {
			recv2_state = FSA_INIT;               //���ճ�ʱ, �ָ�����ʼ״̬			
		}
	}
}