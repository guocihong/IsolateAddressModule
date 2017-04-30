#include "system_init.h"

/*
电源隔离型单RS485总线地址模块
*/

void main(void)
{
    //系统初始化
    system_init();

    //打开总中断
    Enable_interrupt();

    //使用看门狗
    Wdt_enable();// 2.276s 

    while(1) {	
		//解析2路开关量的状态
		adc_task();
		
        //解析uart接收的命令
        uart_task();
		
		//喂狗
        Wdt_refresh();
    }
}