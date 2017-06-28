#include "system_init.h"

static void gpio_init(void);
static void get_defence_info(void);
static void get_config_info(void);

/* System */
extern data   Byte     gl_comm_addr;                         //RS485通信地址
extern data   Byte     gl_reply_tick;                        //设置返回延时


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
	//设置P10、P11为高阻输入，并且做为AD使用
	P1M1 = 0x03;
	P1M0 = 0x00;
	P1ASF = 0x03;
	
	//设置P2口为准双向口
	P2M1 = 0x00;
	P2M0 = 0x00;

	//设置P36为推挽输出
	P3M1 = 0x00;
	P3M0 = 0x40;

	//设置P4口为准双向口
	P4M1 = 0x00;
	P4M0 = 0x00;
	
	//初始化设置
	bRS485_Ctrl = 0;  //RS485发送禁止（处于接收状态）
}


static void get_defence_info(void)
{
	Uint16 i;
	Byte   j;
	Byte   temp;
	
	//读取RS485通信地址
	i = 0;
	temp = P2;
	do {
		//延时
		j = 255;
		while (j>0)  j--;
		if (temp == P2) { //读到相同的值
			i++;
		} else { //读到不同的值
			i = 0;
			temp = P2;
		}
	} while (i < 8);
	gl_comm_addr = ~temp;    //本模块RS485通信地址
	//使用8位拨码开关，地址范围为 0x00 ~ 0xFF,总线设备地址要求从0x10开始
	if ((gl_comm_addr < 0x10) || (gl_comm_addr == 0xFF) || (gl_comm_addr == 0xFE)) {
		//设备地址必须为 0x10 ~ 0xFD 之间(含),0xFF为广播地址,0xFE表示未定义地址
		gl_comm_addr = CMD_ADDR_UNSOLV;
	}
}

static void get_config_info(void)
{
	Byte temp;
	
	//使能Flash访问
	flash_enable();
	
    //读取延时时间
    temp = flash_read(EEPROM_SECTOR1);
    if (temp == 0x5A) { //有有效设置
        gl_reply_tick = flash_read(EEPROM_SECTOR1 + 1);
    } else {	//无有效设置
        gl_reply_tick = 0;
    }
	
	//禁止Flash访问
	flash_disable();
}