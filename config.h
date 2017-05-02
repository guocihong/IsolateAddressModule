#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "STC12C5A60S2.h"
#include "compiler.h"
#include <intrins.h>
#include <string.h>

/* Scheduler Tick */
#define SCHEDULER_TICK     5                                     // unit is ms
#define REPLY_DLY          (100/SCHEDULER_TICK)                  //�յ�PC������Ӧ����ʱ
#define ALARM_TEMPO        (10000/SCHEDULER_TICK)                 //�����źų���ʱ��

/* ϵͳ����� */
#define CMD_ADDR_DEF     0x30    //ȱʡ������ַ
#define CMD_ADDR_BC      255     //�㲥��ַ
#define CMD_ADDR_UNSOLV  254     //δ��¼��δ��ȷ���õĵ�ַ

//˫������ַģ��
#define CMD_DADDR_qSTAT  0xE0    //ѯ�ʷ���״̬
#define CMD_DADDR_qPARA  0xE1    //ѯ�ʵ�ַ

#define CMD_ACK_OK       0x00    
#define CMD_DADDR_aSTAT  0xF0    //Ӧ�� - ��������
#define CMD_DADDR_aPARA  0xF1    //Ӧ�� - ����ѯ��/����

//����/����MODģ��
#define CMD_ZL_PRE       0xE8    //����/����ר�������־

/* AD */
typedef struct strAD_Sample
{ //ÿ�����ֵ
  Uint16   val;                          //��ǰ����ֵ
  Uint8    index;                        //ͨ���ţ���Χ0 ~ 12
  Byte     valid;                        //�������ݴ����־: 0 - �Ѵ�������д����ֵ; 1 - ��ֵ���ȴ�����                                    
}sAD_Sample;

typedef struct strAD_Sum
{ //����ֵ�ۼӺ�
  Uint16   sum;                          //�ۼƺ� (����64��,�������)
  Uint8    point;                        //�Ѳ�������
}sAD_Sum;

//for Uart
#define	FRAME_STX           0x16         // Frame header
#define	MAX_RecvFrame       50           // ���ջ�������С
#define	MAX_TransFrame      50           // ���ͻ�������С
#define RECV_TIMEOUT        4            // �ֽڼ�����ʱ����, ��λΪtick
                                         // ��Сֵ����Ϊ1, ���Ϊ0���ʾ�����г�ʱ�ж�                                    
/* state constant(�����ڽ���) */
#define FSA_INIT            0            //�ȴ�֡ͷ
#define FSA_ADDR_D          1            //�ȴ�Ŀ�ĵ�ַ
#define FSA_ADDR_S          2            //�ȴ�Դ��ַ
#define FSA_LENGTH          3            //�ȴ������ֽ�
#define FSA_DATA            4            //�ȴ����(���� ����ID �� ����)
#define FSA_CHKSUM          5            //�ȴ�У���

/* Uart Queue */
typedef struct strUART_Q
{
  Byte  flag;                            //״̬�� 0 - ���У� 1 - �ȴ����ͣ� 2 - ���ڷ���; 3 - �ѷ��ͣ��ȴ�Ӧ��
  Byte  tdata[MAX_TransFrame];           //���ݰ�,������У����(���һ��У���ֽڿ��Բ���ǰ���㣬���ڷ���ʱ�߷��ͱ߼���)
  Byte  len;					         //���ݰ���Ч����(��У���ֽ�)
}sUART_Q;

#define UART_QUEUE_NUM      7            //UART ������

#define bRS485_Ctrl         P36          //���������RS485����ʹ��: 1-������; 0 - ��ֹ����(����)

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