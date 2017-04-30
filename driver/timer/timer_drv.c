#include "driver/timer/timer_drv.h"
#include "driver/wdt/wdt_drv.h"
#include "driver/adc/adc_drv.h"

/* UART2 */
extern xdata  Byte         recv2_state;                 // receive state
extern xdata  Byte         recv2_timer;                 // receive time-out, 用于字节间超时判定

/* System */
extern data   Uint16       gl_ack_tick;     	        //上位机485口应答延时计时 tick
extern data   Byte         watch_dog_tick;

/* AD sample */
extern  data  Byte         ad_index;                    //正在采样的通道号，范围0-1
extern  data  sAD_Sample   ad_sample;                   //保存当前采样值
extern  data  Uint16       ad_alarm_tick[2];            //各通道报警计时tick

void timer0_init(void)   // 5ms@22.1184MHz
{    
	//watch_dog_tick = 0xFF;
	
    // 定时器0初始化
	AUXR &= 0x7F;		 // 设置为12T模式
	TMOD &= 0xF0;		 // 设置为工作模式1
	TMOD |= 0x01;
	TL0 = 0x00;		     // 设置定时初值
	TH0 = 0xDC;		     // 设置定时初值
	//TH0 = 0x28;		     // 设置定时初值
	TF0 = 0;		     // 清除TF0标志
    ET0 = 1;             // 使能T0中断允许位
    IPH |= (1 << 1);
    PT0 = 0;             // 设置中断优先级为优先级2
	TR0 = 1;		     // 定时器0开始计时
	
	// 启动AD转换
	ADC_CONTR &= 0xF8;   //选中P10做为AD转换
    _nop_();
    _nop_();
    _nop_();
    _nop_();
	ADC_CONTR |= ADC_START;
}

void timer0_isr(void) interrupt TF0_VECTOR using 0
{	      
	Byte i;
	
    // 重装初值
    TR0 = 0;
	TL0 = 0x00;		                              // 设置定时初值
	TH0 = 0xDC;		                              // 设置定时初值
	//TH0 = 0x28;		                              // 设置定时初值
	TR0 = 1;		                              // 定时器0开始计时
        
	// AD转换完成,将ADC_FLAG转换完成标志清零
    ADC_CONTR &= ~ADC_FLAG;

	// 读AD值
	if (ad_sample.valid == FALSE) {
		// 原数据已经被处理, 可以写入新数据
		ad_sample.val   = ADC_RES;                // 读高8位
		ad_sample.val   = ad_sample.val << 2;
		ad_sample.val   += (ADC_RESL & 0x03);     // 得到10bit采样值
		ad_sample.index = ad_index;
		ad_sample.valid = TRUE;
			        
		// 启动下一通道采样
		if (ad_index == 1) {
			ad_index = 0;
		} else {
			ad_index++;
		}

		// 选择模拟输入
		if (ad_index == 0) {//选中P10做为AD输入
			ADC_CONTR &= 0xF8;
		} else if (ad_index == 1) {//选中P11做为AD输入
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
   		ADC_CONTR |= ADC_POWER_ENABLE | ADC_SPEED_540; // 启动转换
        _nop_();
        _nop_();
        _nop_();
        _nop_();
        ADC_CONTR |=  ADC_START;
	}
	
	if (gl_ack_tick > 0) {
		gl_ack_tick--;                            //应答延时计时
	}
	
	for (i = 0; i < 2; i++) {
		if (ad_alarm_tick[i] > 0) {
			ad_alarm_tick[i]--;
		}
	}
		
	/*
	if (watch_dog_tick > 0) {
		watch_dog_tick--;
		
		//喂狗
        Wdt_refresh();
	}
	*/
	
	// UART2字节之间接收超时
	if (recv2_state != FSA_INIT) { 
		//非初始状态，需要检测是否超时
		if (recv2_timer > 0) {
			recv2_timer--;
		}
		
		if (recv2_timer == 0) {
			recv2_state = FSA_INIT;               //接收超时, 恢复至初始状态			
		}
	}
}