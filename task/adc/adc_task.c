#include "task/adc/adc_task.h"

/* AD sample */
extern  data  Byte         ad_equ_pum;                  //每个通道采样多次求平均值

extern  data  Byte         ad_index;                    //正在采样的通道号, 取值范围0~1
extern  data  sAD_Sample   ad_sample;                   //保存当前采样值
extern  data  sAD_Sum      ad_samp_equ[2];              //均衡去噪声求和
extern  data  Uint16       ad_chn_sample[2];            //最新一轮采样值（已均衡去噪声，每通道一个点）
extern  data  Byte         ad_chn_over[2];              //各通道连续采样点(均衡后)的阀值判定： 0 - 范围内； 1 - 超阀值
extern  data  Byte         ad_alarm_exts;               //报警标志：00-无报警,01-1#防区报警，02-2#防区报警，03-1#防区和2#防区同时报警
extern  data  Uint16       ad_alarm_tick[2];            //各通道报警计时tick
extern  data  Byte         alarm_host_read[2];          //报警主机是否读取了本设备的报警状态

void adc_task_init(void)
{
	Byte i;

	//相关变量初始化
	ad_index        = 0;
	ad_sample.valid = 0;                     //空闲，可以写入新值
	
	ad_alarm_exts = 0;
	alarm_host_read[0] = 0;
	alarm_host_read[1] = 0;

	for (i = 0; i < 2; i++) {
		ad_samp_equ[i].sum       = 0;        //均衡去噪声求和
		ad_samp_equ[i].point     = 0;
		ad_chn_sample[i]         = 0;        //最新一轮采样值
		ad_chn_over[i]           = 0;        //各通道连续采样点(均衡后)的阀值判定：均在范围内
		ad_alarm_tick[i]         = 0;
	}
    
	//adc硬件初始化
	adc_init();
}

void adc_task(void)
{
	Byte    index;          //采样通道号
	Uint16  val_temp;       //新送入的10bit采样值,  后作临时变量
	Uint16  val;            //4点均衡后得到的平均采样值, 作为一个可进行超限判断的最小点

	//有新采样数据到达
	if (ad_sample.valid) {  
		// 0. 保存到临时变量
		val_temp = ad_sample.val;
		index    = ad_sample.index;

		// 1. 保存到均衡去噪声求和中
		ad_samp_equ[index].sum += val_temp;
		ad_samp_equ[index].point++;

		// 2. 当前通道是否满足去噪声点数
		if (ad_samp_equ[index].point == 4) {
			// 已满去噪声点数，可求出均衡后的一个点
			// 2.a 求出对应通道的一个采样点
			val = ad_samp_equ[index].sum / 4;  //求平均值

			// 2.b 清零当前通道的去噪声求和结构
			ad_samp_equ[index].sum = 0;
			ad_samp_equ[index].point = 0;

			// 2.c 保存实时采样值
			ad_chn_sample[index] = val;   //保存到最新一轮采样值数组中
			
			// 2.d 判断当前通道是否报警
			ad_chn_over[index] = ad_chn_over[index] << 1;   //Bit0填0，因此缺省在允许范围内
			
			if (index == 0) {//1#防区
				//a. 是否超上下限阀值
				if (!(((val >= TH_A1_EOL_MIN) && (val <= TH_A1_EOL_MAX)) || 
					   ((val >= TH_A1_NC_MIN)  && (val <= TH_A1_NC_MAX)))) { 
					//报警
					ad_chn_over[index] |= 0x01;  	  
				}
			} else if (index == 1) {//2#防区
				//a. 是否超上下限阀值
				if (val >= TH_A2_MAX) { 
					//报警
					ad_chn_over[index] |= 0x01;  	  
				}
			}
			
			//连续4点超范围，此通道有报警
			if ((ad_chn_over[index] & 0x0F) == 0x0F) {
				//超出允许范围，置标志
				ad_alarm_exts |= (1 << index);

				//报警计时tick
				ad_alarm_tick[index] = ALARM_TEMPO;                     
			} else if ((ad_chn_over[index] & 0x0F) == 0x00) {//无报警
				//检查报警时间是否已到,或者报警主机是否读取了本设备的报警状态
				if ((ad_alarm_tick[index] == 0) || (alarm_host_read[index] == 1)) {
					//报警已经到最大报警时间或者报警主机是读取了本设备的报警状态, 停止报警
					ad_alarm_exts &= ~(1 << index);
					
					//复位
					alarm_host_read[index] = 0;
				}
			}
		}

		//3.当前采样值处理完毕，允许新的采样值输入
		ad_sample.valid = FALSE;
	}
}