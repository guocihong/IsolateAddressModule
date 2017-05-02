#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "STC12C5A60S2.h"
#include "compiler.h"
#include <intrins.h>
#include <string.h>

/* Scheduler Tick */
#define SCHEDULER_TICK     5                                     // unit is ms
#define REPLY_DLY          (100/SCHEDULER_TICK)                  //收到PC命令后的应答延时
#define ALARM_TEMPO        (10000/SCHEDULER_TICK)                 //报警信号持续时间

/* 系统命令定义 */
#define CMD_ADDR_DEF     0x30    //缺省主机地址
#define CMD_ADDR_BC      255     //广播地址
#define CMD_ADDR_UNSOLV  254     //未烧录或未正确设置的地址

//双防区地址模块
#define CMD_DADDR_qSTAT  0xE0    //询问防区状态
#define CMD_DADDR_qPARA  0xE1    //询问地址

#define CMD_ACK_OK       0x00    
#define CMD_DADDR_aSTAT  0xF0    //应答 - 防区报警
#define CMD_DADDR_aPARA  0xF1    //应答 - 参数询问/设置

//张力/脉冲MOD模块
#define CMD_ZL_PRE       0xE8    //张力/脉冲专用命令标志

/* AD */
typedef struct strAD_Sample
{ //每点采样值
  Uint16   val;                          //当前采样值
  Uint8    index;                        //通道号，范围0 ~ 12
  Byte     valid;                        //采样数据处理标志: 0 - 已处理，可以写入新值; 1 - 新值，等待处理                                    
}sAD_Sample;

typedef struct strAD_Sum
{ //采样值累加和
  Uint16   sum;                          //累计和 (最多达64点,不会溢出)
  Uint8    point;                        //已采样点数
}sAD_Sum;

//for Uart
#define	FRAME_STX           0x16         // Frame header
#define	MAX_RecvFrame       50           // 接收缓存区大小
#define	MAX_TransFrame      50           // 发送缓存区大小
#define RECV_TIMEOUT        4            // 字节间的最大时间间隔, 单位为tick
                                         // 最小值可以为1, 如果为0则表示不进行超时判定                                    
/* state constant(仅用于接收) */
#define FSA_INIT            0            //等待帧头
#define FSA_ADDR_D          1            //等待目的地址
#define FSA_ADDR_S          2            //等待源地址
#define FSA_LENGTH          3            //等待长度字节
#define FSA_DATA            4            //等待命令串(包括 命令ID 及 参数)
#define FSA_CHKSUM          5            //等待校验和

/* Uart Queue */
typedef struct strUART_Q
{
  Byte  flag;                            //状态： 0 - 空闲； 1 - 等待发送； 2 - 正在发送; 3 - 已发送，等待应答
  Byte  tdata[MAX_TransFrame];           //数据包,不包含校验码(最后一个校验字节可以不提前计算，而在发送时边发送边计算)
  Byte  len;					         //数据包有效长度(含校验字节)
}sUART_Q;

#define UART_QUEUE_NUM      7            //UART 队列数

#define bRS485_Ctrl         P36          //推挽输出，RS485发送使能: 1-允许发送; 0 - 禁止发送(接收)

/* Alarm Threshold */
#define TH_A1_EOL_MAX  270
#define TH_A1_EOL_MIN  200
#define TH_A1_NC_MAX   590
#define TH_A1_NC_MIN   480

#define TH_A2_MAX  512
#define TH_A2_MIN  20

/* interrupt enable */
#define Enable_interrupt()  (EA = 1)
#define Disable_interrupt() (EA = 0)

#endif