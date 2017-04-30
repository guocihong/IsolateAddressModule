#include "config.h"

/* UART2 */	
xdata  Byte         recv2_buf[MAX_RecvFrame];             // receiving buffer
xdata  Byte         recv2_state;                          // receive state
xdata  Byte         recv2_timer;                          // receive time-out, 用于字节间超时判定
xdata  Byte         recv2_chksum;                         // computed checksum
xdata  Byte         recv2_ctr;                            // reveiving pointer
                    
xdata  Byte         trans2_buf[MAX_TransFrame];           // uart transfer message buffer
xdata  Byte         trans2_ctr;                           // transfer pointer
xdata  Byte         trans2_size;                          // transfer bytes number
xdata  Byte         trans2_chksum;                        // computed check-sum of already transfered message

xdata  Byte         uart2_q_index;                        // 正在发送某队列项的序号：若为0xFF, 表示没有任何项进入发送流程
xdata  sUART_Q      uart2_send_queue[UART_QUEUE_NUM];     // 串口队列
xdata  sUART_Q      uart2_recv_queue[UART_QUEUE_NUM];     // 串口队列


/* System */
data   Uint16       gl_ack_tick;     	                  //上位机485口应答延时计时 tick
data   Byte         gl_reply_tick;                        //设备返回延时
data   Byte         gl_comm_addr;                         //本模块485通信地址
data   Byte         watch_dog_tick;

/* AD sample */
data   Byte         ad_index;                             //正在采样的通道号，范围0-1
data   sAD_Sample   ad_sample;                            //保存当前采样值
data   sAD_Sum      ad_samp_equ[2];                       //均衡去噪声求和
data   Uint16       ad_chn_sample[2];                     //最新一轮采样值（已均衡去噪声，每通道一点）
data   Byte         ad_equ_pum;                           //每个通道采样多次求平均值
data   Byte         ad_chn_over[2];                       //各通道连续采样点(均衡后)的阀值判定： 0 - 范围内； 1 - 超阀值                              
data   Byte         ad_alarm_exts;                        //报警标志：00-无报警, 01-1#防区报警, 02-2#防区报警, 03-1#和2#防区同时报警                   
data   Uint16       ad_alarm_tick[2];                     //各通道报警计时tick

