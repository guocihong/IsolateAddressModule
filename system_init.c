#include "system_init.h"

static void gpio_init(void);
static void get_defence_info(void);
static void get_config_info(void);

/* System */
extern data   Byte     gl_comm_addr;                         //RS485ͨ�ŵ�ַ
extern data   Byte     gl_reply_tick;                        //���÷�����ʱ


void system_init(void)
{   
	gpio_init();
	
	get_defence_info();
	get_config_info();

	uart_task_init();
	adc_task_init();
	
	timer0_init();
}

static void gpio_init(void)
{        
	//����P10��P11Ϊ�������룬������ΪADʹ��
	P1M1 = 0x03;
	P1M0 = 0x00;
	P1ASF = 0x03;
	
	//����P2��Ϊ׼˫���
	P2M1 = 0x00;
	P2M0 = 0x00;

	//����P36Ϊ�������
	P3M1 = 0x00;
	P3M0 = 0x40;

	//����P4��Ϊ׼˫���
	P4M1 = 0x00;
	P4M0 = 0x00;
	
	//��ʼ������
	bRS485_Ctrl = 0;  //RS485���ͽ�ֹ�����ڽ���״̬��
}


static void get_defence_info(void)
{
	Uint16 i;
	Byte   j;
	Byte   temp;
	
	//��ȡRS485ͨ�ŵ�ַ
	i = 0;
	temp = P2;
	do {
		//��ʱ
		j = 255;
		while (j>0)  j--;
		if (temp == P2) { //������ͬ��ֵ
			i++;
		} else { //������ͬ��ֵ
			i = 0;
			temp = P2;
		}
	} while (i < 8);
	gl_comm_addr = ~temp;    //��ģ��RS485ͨ�ŵ�ַ
	//ʹ��8λ���뿪�أ���ַ��ΧΪ 0x00 ~ 0xFF,�����豸��ַҪ���0x10��ʼ
	if ((gl_comm_addr < 0x10) || (gl_comm_addr == 0xFF) || (gl_comm_addr == 0xFE)) {
		//�豸��ַ����Ϊ 0x10 ~ 0xFD ֮��(��),0xFFΪ�㲥��ַ,0xFE��ʾδ�����ַ
		gl_comm_addr = CMD_ADDR_UNSOLV;
	}
}

static void get_config_info(void)
{
	Byte temp;
	
	//ʹ��Flash����
	flash_enable();
	
    //��ȡ��ʱʱ��
    temp = flash_read(EEPROM_SECTOR1);
    if (temp == 0x5A) { //����Ч����
        gl_reply_tick = flash_read(EEPROM_SECTOR1 + 1);
    } else {	//����Ч����
        gl_reply_tick = 0;
    }
	
	//��ֹFlash����
	flash_disable();
}