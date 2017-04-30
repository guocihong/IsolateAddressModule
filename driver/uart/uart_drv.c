#include "driver/uart/uart_drv.h"

#define S2TI   0x02
#define S2RI   0x01

/* UART2 */
extern xdata  Byte     recv2_buf[MAX_RecvFrame];             // receiving buffer
extern xdata  Byte     recv2_state;                          // receive state
extern xdata  Byte     recv2_timer;                          // receive time-out, �����ֽڼ䳬ʱ�ж�
extern xdata  Byte     recv2_chksum;                         // computed checksum
extern xdata  Byte     recv2_ctr;                            // reveiving pointer

extern xdata  Byte     trans2_buf[MAX_TransFrame];           // uart transfer message buffer
extern xdata  Byte     trans2_ctr;                           // transfer pointer
extern xdata  Byte     trans2_size;                          // transfer bytes number
extern xdata  Byte     trans2_chksum;                        // computed check-sum of already transfered message

extern xdata  Byte     uart2_q_index;                        // ���ڷ���ĳ���������ţ���Ϊ0xFF, ��ʾû���κ�����뷢������
extern xdata  sUART_Q  uart2_send_queue[UART_QUEUE_NUM];     // ���ڷ��Ͷ���
extern xdata  sUART_Q  uart2_recv_queue[UART_QUEUE_NUM];     // ���ڽ��ն���

/* System */
extern data   Uint16   gl_ack_tick;     	                 //��λ��485��Ӧ����ʱ��ʱ tick
extern data   Byte     gl_comm_addr;                         //RS485ͨ�ŵ�ַ
extern data   Byte     gl_reply_tick;                        //���÷�����ʱ

#define UART2_RECEIVE_ENABLE()    (S2CON |= 0x10)
#define UART2_RECEIVE_DISABLE()   (S2CON &= 0xEF)

extern Byte uart2_get_recv_buffer(void);

void uart_init(void)    //9600bps@22.1184MHz
{
	//uart2Ӳ����ʼ��
	AUXR1 |= 0x10;      //����2��P1���л���P4��
	S2CON = 0x50;		//8λ����,�ɱ䲨����,����ʹ��
	AUXR &= 0xF7;		//�����ʲ�����
	AUXR &= 0xFB;		//���������ʷ�����ʱ��ΪFosc/12,��12T
	BRT = 0xFA;		    //�趨���������ʷ�������װֵ
	AUXR |= 0x10;		//�������������ʷ�����
	IE2 |= 0x01;        //ʹ�ܴ���2�ж�
	IP2H &= 0xFE;       //�����ж����ȼ�Ϊ���ȼ�1
	IP2 |= 0x01;
		
	//RS485�ӿڳ�ʼ��
	bRS485_Ctrl = 0;    //��˫��������

	_nop_();
	_nop_();
	_nop_();
	_nop_();
}

void uart2_isr(void) interrupt SIO2_VECTOR using 1
{
	volatile Byte nop_num;
	Byte c,i;

	if (S2CON & S2TI) { //UART2�����ж�
		trans2_ctr++;   //ȡ��һ��������index
		if (trans2_ctr < trans2_size) { //δ�������
			if (trans2_ctr == (trans2_size - 1)) { //�Ѿ�ָ��У���ֽ�
				S2BUF = trans2_chksum;    //����У���ֽ�
			} else { //��У���ֽ�, ��Ҫ���Ͳ�����checksum
				S2BUF = trans2_buf[trans2_ctr];
				if (trans2_ctr > 0) { //����check_sum
					trans2_chksum += trans2_buf[trans2_ctr];   //����chksum
				}
			}
		} else { //�Ѿ�ȫ���������(��У���ֽ�)�������÷���������
			//Ŀǰ��ƣ�������ȴ�Ӧ��, �����ͷŸö�����
			if (uart2_q_index < UART_QUEUE_NUM) {
				uart2_send_queue[uart2_q_index].flag = 0;   //�ö��������
			}
			
			uart2_q_index = 0xFF;	 //�޶������ڷ���
			nop_num = 100;
			while (nop_num--);
			bRS485_Ctrl = 0;	     //��ֹ����, תΪ����
			UART2_RECEIVE_ENABLE();  //UART2�������		
		}
        
		S2CON &= ~S2TI;   //must clear by user software
	}
	
	if (S2CON & S2RI) { //�����ж�
		c = S2BUF;
		switch (recv2_state)
		{
		case FSA_INIT://�Ƿ�Ϊ֡ͷ
			if (c == FRAME_STX) { //Ϊ֡ͷ, ��ʼ�µ�һ֡
				recv2_ctr = 0;
				recv2_chksum = 0;
				recv2_timer = RECV_TIMEOUT;
				recv2_state = FSA_ADDR_D;
			}
			break;

		case FSA_ADDR_D://ΪĿ�ĵ�ַ, ��ʼ���沢����Ч���
			recv2_buf[recv2_ctr++] = c;
			recv2_chksum += c;
			recv2_timer = RECV_TIMEOUT;
			recv2_state = FSA_ADDR_S;
			break;

		case FSA_ADDR_S://ΪԴ��ַ
			recv2_buf[recv2_ctr++] = c;
			recv2_chksum += c;
			recv2_timer = RECV_TIMEOUT;
			recv2_state = FSA_LENGTH;
			break;

		case FSA_LENGTH://Ϊ�����ֽ�
			if ((c > 0) && (c < (MAX_RecvFrame - 3))) { //��Ч��
				recv2_buf[recv2_ctr++] = c;    //�������ֽڱ��泤��
				recv2_chksum += c;
				recv2_timer = RECV_TIMEOUT;
				recv2_state = FSA_DATA;
			} else {	//����Ч��
				recv2_state = FSA_INIT;
			}
			break;

		case FSA_DATA://��ȡ���
			recv2_buf[recv2_ctr] = c;
			recv2_chksum += c;   //����У���
			if (recv2_ctr == (recv2_buf[2] + 2)) { //�Ѿ��յ�ָ�����ȵ���������
				recv2_state = FSA_CHKSUM;
			} else {	//��δ����
				recv2_ctr++;
			}
			recv2_timer = RECV_TIMEOUT;
			break;

		case FSA_CHKSUM://���У���ֽ�
			if (recv2_chksum == c) {//�Ѿ��յ�����һ֡
				//ֻ���汨���������͵Ĺ㲥����͵�����������豸���ص����ݰ�ȫ������
				if ((recv2_buf[0] == CMD_ADDR_BC) || (recv2_buf[0] == gl_comm_addr)) {
				    i = uart2_get_recv_buffer();
					if (i < UART_QUEUE_NUM) {//�ҵ��˿���buffer, д��data
						memcpy(uart2_recv_queue[i].tdata, recv2_buf, recv2_buf[2] + 3);
						
						gl_ack_tick = 100;
					}
				}
			}
		default://��λ
			recv2_state = FSA_INIT;
			break;
		}
        
		S2CON &= ~S2RI;     //must clear by user software
	}
}

void uart2_start_trans(void)
{ 	
	volatile Byte nop_num = 100;

	UART2_RECEIVE_DISABLE();
	_nop_();
	_nop_();
	bRS485_Ctrl = 1;    //������
	while (nop_num--);
	trans2_chksum = 0;
	trans2_ctr = 0;
	S2BUF = trans2_buf[trans2_ctr];
}