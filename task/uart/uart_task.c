#include "task/uart/uart_task.h"

/* UART2 */
extern xdata  Byte     recv2_buf[MAX_RecvFrame];             // receiving buffer
extern xdata  Byte     recv2_state;                          // receive state
extern xdata  Byte     recv2_timer;                          // receive time-out, 用于字节间超时判定
extern xdata  Byte     recv2_chksum;                         // computed checksum
extern xdata  Byte     recv2_ctr;                            // reveiving pointer

extern xdata  Byte     trans2_buf[MAX_TransFrame];           // uart transfer message buffer
extern xdata  Byte     trans2_ctr;                           // transfer pointer
extern xdata  Byte     trans2_size;                          // transfer bytes number
extern xdata  Byte     trans2_chksum;                        // computed check-sum of already transfered message

extern xdata  Byte     uart2_q_index;                        // 正在发送某队列项的序号：若为0xFF, 表示没有任何项进入发送流程
extern xdata  sUART_Q  uart2_send_queue[UART_QUEUE_NUM];     // 串口发送队列
extern xdata  sUART_Q  uart2_recv_queue[UART_QUEUE_NUM];     // 串口接收队列

/* System */
extern data   Uint16   gl_ack_tick;     	                 //上位机485口应答延时计时 tick
extern data   Byte     gl_comm_addr;                         //RS485通信地址
extern data   Byte     gl_reply_tick;                        //设置返回延时
extern data   Byte     watch_dog_tick;

/* AD sample */
extern data   Byte     ad_alarm_exts;                        //报警标志：00-无报警,01-1#防区报警，02-2#防区报警，03-1#防区和2#防区同时报警


void uart_task_init(void)
{
	Byte i;

	//uart2相关变量初始化
	recv2_state    = FSA_INIT;
	recv2_timer    = 0;
	recv2_ctr      = 0;
	recv2_chksum   = 0;

	trans2_size    = 0;
	trans2_ctr     = 0;

	for (i = 0; i < UART_QUEUE_NUM; i++) {
		uart2_send_queue[i].flag = 0; //均空闲
		uart2_recv_queue[i].flag = 0; //均空闲
	}
	uart2_q_index = 0xFF;             //无队列项进入发送流程

	//UART硬件初始化
	uart_init();                      //已经准备好串口收发，只是还未使能全局中断
}

void uart_task(void)
{
	Byte i,j;
	Byte *ptr;
	
	//1. UART2 队列处理
	//找是否有等待处理的项
	for (j = 0; j < UART_QUEUE_NUM; j++) {
		if (uart2_recv_queue[j].flag == 1) {//有等待处理的项
			ptr = uart2_recv_queue[j].tdata;
			
			//是否需要执行本命令
			if ((ptr[0] == CMD_ADDR_BC) || (ptr[0] == gl_comm_addr)) {
				//广播地址或指定本设备, 需要执行
				switch (ptr[3])
				{
				case CMD_DADDR_qSTAT://询问防区状态 - 报告给上位机
					//在UART2发送队列中找空闲Buffer
					i = uart2_get_send_buffer();
					if (i < UART_QUEUE_NUM) {
						//找到了空闲buffer, 准备应答					
						uart2_send_queue[i].tdata[0] = FRAME_STX;	       //帧头
						uart2_send_queue[i].tdata[1] = ptr[1];	           //目的地址
						uart2_send_queue[i].tdata[2] = gl_comm_addr;       //源地址
						if (gl_comm_addr == CMD_ADDR_UNSOLV) {             //本设备无有效地址
							//只回参数应答
							uart2_send_queue[i].tdata[3] = 1;
							uart2_send_queue[i].tdata[4] = CMD_DADDR_aPARA;
							uart2_send_queue[i].len = 6;
						} else { //有有效地址,回防区状态
							if ((ad_alarm_exts & 0x03) == 0x00) { //2个防区均无报警
								uart2_send_queue[i].tdata[3] = 1;
								uart2_send_queue[i].tdata[4] = CMD_ACK_OK;
								uart2_send_queue[i].len = 6;
							} else { //有报警
								uart2_send_queue[i].tdata[3] = 2;
								uart2_send_queue[i].tdata[4] = CMD_DADDR_aSTAT;
								uart2_send_queue[i].tdata[5] = (ad_alarm_exts & 0x03);
								uart2_send_queue[i].len = 7;
							}
						}
					}
					
					break;

				case CMD_DADDR_qPARA://询问地址- 报告给上位机
					//在UART2发送队列中找空闲Buffer
					i = uart2_get_send_buffer();
					if (i < UART_QUEUE_NUM) {
						//找到了空闲buffer, 准备应答					
						uart2_send_queue[i].tdata[0] = FRAME_STX;	               //帧头
						uart2_send_queue[i].tdata[1] = ptr[1];	                   //目的地址
						uart2_send_queue[i].tdata[2] = gl_comm_addr;	           //源地址
						uart2_send_queue[i].tdata[3] = 1;                          //命令长度
						uart2_send_queue[i].tdata[4] = CMD_DADDR_aPARA;            //命令ID
						uart2_send_queue[i].len = 6;
					}
					
					break;
					
				case 0xE3://设置延时时间
                    //1. 写入flash
                    flash_enable();                              
                    flash_erase(EEPROM_SECTOR1);                                        
                    flash_write(ptr[4], EEPROM_SECTOR1 + 1);  
                    flash_write(0x5a, EEPROM_SECTOR1);                                                              
                    flash_disable();

                    //2. 更新变量
                    gl_reply_tick = ptr[4];
				
                    break;

                case 0xE4://读取延时时间
                    //在UART3队列中找空闲Buffer
                    i = uart2_get_send_buffer();
                    if (i < UART_QUEUE_NUM) { //找到了空闲buffer, 准备应答                       
                        uart2_send_queue[i].tdata[0] = FRAME_STX;	                   //帧头
                        uart2_send_queue[i].tdata[1] = ptr[1];	                       //目的地址
                        uart2_send_queue[i].tdata[2] = gl_comm_addr;	               //源地址																 
                        uart2_send_queue[i].tdata[3] = 2;
                        uart2_send_queue[i].tdata[4] = 0xF4;
                        uart2_send_queue[i].tdata[5] = gl_reply_tick;
                        uart2_send_queue[i].len = 7;														
                    }
                    
                    break;	
				}
				
				//设置应答延时
                Disable_interrupt();
                //gl_ack_tick = gl_comm_addr - 15;
				gl_ack_tick = (Uint16)(REPLY_DLY + (Uint16)((gl_comm_addr - 16) * gl_reply_tick) / SCHEDULER_TICK);

				// 重装初值
				TR0 = 0;
				TL0 = 0x00;		                              // 设置定时初值
				TH0 = 0xDC;		                              // 设置定时初值
				//TH0 = 0x28;		                          // 设置定时初值
				TR0 = 1;		                              // 定时器0开始计时
				
                Enable_interrupt();
			}

			//处理完成,释放该队列项
			uart2_recv_queue[j].flag = 0;
			break;
		}
	}

	//2. UART2 队列发送
	if ((uart2_q_index == 0xFF) && (recv2_state == FSA_INIT)) {
		//UART2无进入发送流程的队列项, 找是否有等待发送的项
		for (i = 0; i < UART_QUEUE_NUM; i++) {
			if ((uart2_send_queue[i].flag == 1) && (gl_ack_tick == 0)) {
				//重新赋值
				//watch_dog_tick = 0xFF;
				
				//有等待发送的项，安排此项发送
				uart2_send_queue[i].flag = 2;
				uart2_q_index = i;
				memcpy(trans2_buf, uart2_send_queue[i].tdata, uart2_send_queue[i].len - 1);
				trans2_size = uart2_send_queue[i].len;
				uart2_start_trans();
				break;
			}
		}
	}
}

/***************************************************************************
* NAME: uart2_get_send_buffer
*----------------------------------------------------------------------------
* PARAMS:
* return: Byte
*         若返回值 >= UART_QUEUE_NUM, 则表示没有申请到空闲buffer
*----------------------------------------------------------------------------
* PURPOSE: 在串口2队列中寻找空闲队列项，若找到，返回队列项序号(0 ~ (UART_QUEUE_NUM-1))
*****************************************************************************/
Byte uart2_get_send_buffer(void)
{
	Byte i, flag;

	for (i = 0; i < UART_QUEUE_NUM; i++) {
		Disable_interrupt();
		flag = uart2_send_queue[i].flag;
		Enable_interrupt();
		if (flag == 0) {  //已找到空闲Buffer
			uart2_send_queue[i].flag = 1;
			break;
		}
	}
	return i;
}

/***************************************************************************
* NAME: uart2_get_recv_buffer
*----------------------------------------------------------------------------
* PARAMS:
* return: Byte
*         若返回值 >= UART_QUEUE_NUM, 则表示没有申请到空闲buffer
*----------------------------------------------------------------------------
* PURPOSE: 在串口2队列中寻找空闲队列项，若找到，返回队列项序号(0 ~ (UART_QUEUE_NUM-1))
*****************************************************************************/
Byte uart2_get_recv_buffer(void)
{
	Byte i, flag;

	for (i = 0; i < UART_QUEUE_NUM; i++) {
		Disable_interrupt();
		flag = uart2_recv_queue[i].flag;
		Enable_interrupt();
		if (flag == 0) {  //已找到空闲Buffer
			uart2_recv_queue[i].flag = 1;
			break;
		}
	}
	return i;
}