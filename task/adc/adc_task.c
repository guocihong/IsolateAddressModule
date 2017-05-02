#include "task/adc/adc_task.h"

/* AD sample */
extern  data  Byte         ad_equ_pum;                  //ÿ��ͨ�����������ƽ��ֵ

extern  data  Byte         ad_index;                    //���ڲ�����ͨ����, ȡֵ��Χ0~1
extern  data  sAD_Sample   ad_sample;                   //���浱ǰ����ֵ
extern  data  sAD_Sum      ad_samp_equ[2];              //����ȥ�������
extern  data  Uint16       ad_chn_sample[2];            //����һ�ֲ���ֵ���Ѿ���ȥ������ÿͨ��һ���㣩
extern  data  Byte         ad_chn_over[2];              //��ͨ������������(�����)�ķ�ֵ�ж��� 0 - ��Χ�ڣ� 1 - ����ֵ
extern  data  Byte         ad_alarm_exts;               //������־��00-�ޱ���,01-1#����������02-2#����������03-1#������2#����ͬʱ����
extern  data  Uint16       ad_alarm_tick[2];            //��ͨ��������ʱtick
extern  data  Byte         alarm_host_read[2];          //���������Ƿ��ȡ�˱��豸�ı���״̬

void adc_task_init(void)
{
	Byte i;

	//��ر�����ʼ��
	ad_index        = 0;
	ad_sample.valid = 0;                     //���У�����д����ֵ
	
	ad_alarm_exts = 0;
	alarm_host_read[0] = 0;
	alarm_host_read[1] = 0;

	for (i = 0; i < 2; i++) {
		ad_samp_equ[i].sum       = 0;        //����ȥ�������
		ad_samp_equ[i].point     = 0;
		ad_chn_sample[i]         = 0;        //����һ�ֲ���ֵ
		ad_chn_over[i]           = 0;        //��ͨ������������(�����)�ķ�ֵ�ж������ڷ�Χ��
		ad_alarm_tick[i]         = 0;
	}
    
	//adcӲ����ʼ��
	adc_init();
}

void adc_task(void)
{
	Byte    index;          //����ͨ����
	Uint16  val_temp;       //�������10bit����ֵ,  ������ʱ����
	Uint16  val;            //4������õ���ƽ������ֵ, ��Ϊһ���ɽ��г����жϵ���С��

	//���²������ݵ���
	if (ad_sample.valid) {  
		// 0. ���浽��ʱ����
		val_temp = ad_sample.val;
		index    = ad_sample.index;

		// 1. ���浽����ȥ���������
		ad_samp_equ[index].sum += val_temp;
		ad_samp_equ[index].point++;

		// 2. ��ǰͨ���Ƿ�����ȥ��������
		if (ad_samp_equ[index].point == 4) {
			// ����ȥ���������������������һ����
			// 2.a �����Ӧͨ����һ��������
			val = ad_samp_equ[index].sum / 4;  //��ƽ��ֵ

			// 2.b ���㵱ǰͨ����ȥ������ͽṹ
			ad_samp_equ[index].sum = 0;
			ad_samp_equ[index].point = 0;

			// 2.c ����ʵʱ����ֵ
			ad_chn_sample[index] = val;   //���浽����һ�ֲ���ֵ������
			
			// 2.d �жϵ�ǰͨ���Ƿ񱨾�
			ad_chn_over[index] = ad_chn_over[index] << 1;   //Bit0��0�����ȱʡ������Χ��
			
			if (index == 0) {//1#����
				//a. �Ƿ������޷�ֵ
				if (!(((val >= TH_A1_EOL_MIN) && (val <= TH_A1_EOL_MAX)) || 
					   ((val >= TH_A1_NC_MIN)  && (val <= TH_A1_NC_MAX)))) { 
					//����
					ad_chn_over[index] |= 0x01;  	  
				}
			} else if (index == 1) {//2#����
				//a. �Ƿ������޷�ֵ
				if (val >= TH_A2_MAX) { 
					//����
					ad_chn_over[index] |= 0x01;  	  
				}
			}
			
			//����4�㳬��Χ����ͨ���б���
			if ((ad_chn_over[index] & 0x0F) == 0x0F) {
				//��������Χ���ñ�־
				ad_alarm_exts |= (1 << index);

				//������ʱtick
				ad_alarm_tick[index] = ALARM_TEMPO;                     
			} else if ((ad_chn_over[index] & 0x0F) == 0x00) {//�ޱ���
				//��鱨��ʱ���Ƿ��ѵ�,���߱��������Ƿ��ȡ�˱��豸�ı���״̬
				if ((ad_alarm_tick[index] == 0) || (alarm_host_read[index] == 1)) {
					//�����Ѿ�����󱨾�ʱ����߱��������Ƕ�ȡ�˱��豸�ı���״̬, ֹͣ����
					ad_alarm_exts &= ~(1 << index);
					
					//��λ
					alarm_host_read[index] = 0;
				}
			}
		}

		//3.��ǰ����ֵ������ϣ������µĲ���ֵ����
		ad_sample.valid = FALSE;
	}
}