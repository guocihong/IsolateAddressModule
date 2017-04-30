#include "task/uart/uart_task.h"

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
extern data   Byte     watch_dog_tick;

/* AD sample */
extern data   Byte     ad_alarm_exts;                        //������־��00-�ޱ���,01-1#����������02-2#����������03-1#������2#����ͬʱ����


void uart_task_init(void)
{
	Byte i;

	//uart2��ر�����ʼ��
	recv2_state    = FSA_INIT;
	recv2_timer    = 0;
	recv2_ctr      = 0;
	recv2_chksum   = 0;

	trans2_size    = 0;
	trans2_ctr     = 0;

	for (i = 0; i < UART_QUEUE_NUM; i++) {
		uart2_send_queue[i].flag = 0; //������
		uart2_recv_queue[i].flag = 0; //������
	}
	uart2_q_index = 0xFF;             //�޶�������뷢������

	//UARTӲ����ʼ��
	uart_init();                      //�Ѿ�׼���ô����շ���ֻ�ǻ�δʹ��ȫ���ж�
}

void uart_task(void)
{
	Byte i,j;
	Byte *ptr;
	
	//1. UART2 ���д���
	//���Ƿ��еȴ��������
	for (j = 0; j < UART_QUEUE_NUM; j++) {
		if (uart2_recv_queue[j].flag == 1) {//�еȴ��������
			ptr = uart2_recv_queue[j].tdata;
			
			//�Ƿ���Ҫִ�б�����
			if ((ptr[0] == CMD_ADDR_BC) || (ptr[0] == gl_comm_addr)) {
				//�㲥��ַ��ָ�����豸, ��Ҫִ��
				switch (ptr[3])
				{
				case CMD_DADDR_qSTAT://ѯ�ʷ���״̬ - �������λ��
					//��UART2���Ͷ������ҿ���Buffer
					i = uart2_get_send_buffer();
					if (i < UART_QUEUE_NUM) {
						//�ҵ��˿���buffer, ׼��Ӧ��					
						uart2_send_queue[i].tdata[0] = FRAME_STX;	       //֡ͷ
						uart2_send_queue[i].tdata[1] = ptr[1];	           //Ŀ�ĵ�ַ
						uart2_send_queue[i].tdata[2] = gl_comm_addr;       //Դ��ַ
						if (gl_comm_addr == CMD_ADDR_UNSOLV) {             //���豸����Ч��ַ
							//ֻ�ز���Ӧ��
							uart2_send_queue[i].tdata[3] = 1;
							uart2_send_queue[i].tdata[4] = CMD_DADDR_aPARA;
							uart2_send_queue[i].len = 6;
						} else { //����Ч��ַ,�ط���״̬
							if ((ad_alarm_exts & 0x03) == 0x00) { //2���������ޱ���
								uart2_send_queue[i].tdata[3] = 1;
								uart2_send_queue[i].tdata[4] = CMD_ACK_OK;
								uart2_send_queue[i].len = 6;
							} else { //�б���
								uart2_send_queue[i].tdata[3] = 2;
								uart2_send_queue[i].tdata[4] = CMD_DADDR_aSTAT;
								uart2_send_queue[i].tdata[5] = (ad_alarm_exts & 0x03);
								uart2_send_queue[i].len = 7;
							}
						}
					}
					
					break;

				case CMD_DADDR_qPARA://ѯ�ʵ�ַ- �������λ��
					//��UART2���Ͷ������ҿ���Buffer
					i = uart2_get_send_buffer();
					if (i < UART_QUEUE_NUM) {
						//�ҵ��˿���buffer, ׼��Ӧ��					
						uart2_send_queue[i].tdata[0] = FRAME_STX;	               //֡ͷ
						uart2_send_queue[i].tdata[1] = ptr[1];	                   //Ŀ�ĵ�ַ
						uart2_send_queue[i].tdata[2] = gl_comm_addr;	           //Դ��ַ
						uart2_send_queue[i].tdata[3] = 1;                          //�����
						uart2_send_queue[i].tdata[4] = CMD_DADDR_aPARA;            //����ID
						uart2_send_queue[i].len = 6;
					}
					
					break;
					
				case 0xE3://������ʱʱ��
                    //1. д��flash
                    flash_enable();                              
                    flash_erase(EEPROM_SECTOR1);                                        
                    flash_write(ptr[4], EEPROM_SECTOR1 + 1);  
                    flash_write(0x5a, EEPROM_SECTOR1);                                                              
                    flash_disable();

                    //2. ���±���
                    gl_reply_tick = ptr[4];
				
                    break;

                case 0xE4://��ȡ��ʱʱ��
                    //��UART3�������ҿ���Buffer
                    i = uart2_get_send_buffer();
                    if (i < UART_QUEUE_NUM) { //�ҵ��˿���buffer, ׼��Ӧ��                       
                        uart2_send_queue[i].tdata[0] = FRAME_STX;	                   //֡ͷ
                        uart2_send_queue[i].tdata[1] = ptr[1];	                       //Ŀ�ĵ�ַ
                        uart2_send_queue[i].tdata[2] = gl_comm_addr;	               //Դ��ַ																 
                        uart2_send_queue[i].tdata[3] = 2;
                        uart2_send_queue[i].tdata[4] = 0xF4;
                        uart2_send_queue[i].tdata[5] = gl_reply_tick;
                        uart2_send_queue[i].len = 7;														
                    }
                    
                    break;	
				}
				
				//����Ӧ����ʱ
                Disable_interrupt();
                //gl_ack_tick = gl_comm_addr - 15;
				gl_ack_tick = (Uint16)(REPLY_DLY + (Uint16)((gl_comm_addr - 16) * gl_reply_tick) / SCHEDULER_TICK);

				// ��װ��ֵ
				TR0 = 0;
				TL0 = 0x00;		                              // ���ö�ʱ��ֵ
				TH0 = 0xDC;		                              // ���ö�ʱ��ֵ
				//TH0 = 0x28;		                          // ���ö�ʱ��ֵ
				TR0 = 1;		                              // ��ʱ��0��ʼ��ʱ
				
                Enable_interrupt();
			}

			//�������,�ͷŸö�����
			uart2_recv_queue[j].flag = 0;
			break;
		}
	}

	//2. UART2 ���з���
	if ((uart2_q_index == 0xFF) && (recv2_state == FSA_INIT)) {
		//UART2�޽��뷢�����̵Ķ�����, ���Ƿ��еȴ����͵���
		for (i = 0; i < UART_QUEUE_NUM; i++) {
			if ((uart2_send_queue[i].flag == 1) && (gl_ack_tick == 0)) {
				//���¸�ֵ
				//watch_dog_tick = 0xFF;
				
				//�еȴ����͵�����Ŵ����
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
*         ������ֵ >= UART_QUEUE_NUM, ���ʾû�����뵽����buffer
*----------------------------------------------------------------------------
* PURPOSE: �ڴ���2������Ѱ�ҿ��ж�������ҵ������ض��������(0 ~ (UART_QUEUE_NUM-1))
*****************************************************************************/
Byte uart2_get_send_buffer(void)
{
	Byte i, flag;

	for (i = 0; i < UART_QUEUE_NUM; i++) {
		Disable_interrupt();
		flag = uart2_send_queue[i].flag;
		Enable_interrupt();
		if (flag == 0) {  //���ҵ�����Buffer
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
*         ������ֵ >= UART_QUEUE_NUM, ���ʾû�����뵽����buffer
*----------------------------------------------------------------------------
* PURPOSE: �ڴ���2������Ѱ�ҿ��ж�������ҵ������ض��������(0 ~ (UART_QUEUE_NUM-1))
*****************************************************************************/
Byte uart2_get_recv_buffer(void)
{
	Byte i, flag;

	for (i = 0; i < UART_QUEUE_NUM; i++) {
		Disable_interrupt();
		flag = uart2_recv_queue[i].flag;
		Enable_interrupt();
		if (flag == 0) {  //���ҵ�����Buffer
			uart2_recv_queue[i].flag = 1;
			break;
		}
	}
	return i;
}