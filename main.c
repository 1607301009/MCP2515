#include <reg52.h>
#include "MCP2515.H"
#include <stdio.h>


//��������
extern void UART_init(void);
extern void UART_send_str(uint8 d);
extern void UART_send_buffer(uint8 *buffer,uint16 len);

extern void Delay_Nms(uint16 x);

extern uint8 MCP2515_ReadByte(uint8 addr);
extern void MCP2515_Init(uint8 *CAN_Bitrate);
extern void CAN_Send_buffer(uint32 ID,uint8 EXIDE,uint8 DLC,uint8 *Send_data);
extern void CAN_Receive_Buffer(uint8 RXB_CTRL_Address,uint8 *CAN_RX_Buf);


bool CAN_MERRF_Flag = 0;                            //CAN���Ĵ����жϱ�־λ
bool CAN_WAKIF_Flag = 0;                            //CAN�����жϱ�־λ
bool CAN_ERRIF_Flag = 0;                            //CAN�����жϱ�־λ��EFLG �Ĵ������ж���ж�Դ��
bool CAN_TX2IF_Flag = 0;                            //MCP2515���ͻ�����2 ���жϱ�־λ
bool CAN_TX1IF_Flag = 0;                            //MCP2515���ͻ�����1 ���жϱ�־λ
bool CAN_TX0IF_Flag = 0;                            //MCP2515���ͻ�����0 ���жϱ�־λ
bool CAN_RX1IF_Flag = 0;                            //MCP2515���ջ�����1 ���жϱ�־λ
bool CAN_RX0IF_Flag = 0;                            //MCP2515���ջ�����0 ���жϱ�־λ



char putchar(char c)  //printf���������putchar()
{
    UART_send_str(c);
    return c;
}

//MCP2515������	Ҫ����FOSC=8M BRP=0..64 PRSEG=1..8 PHSEG1=3..16 PHSEG2=2..8 SJW=1..4
uint8 code bitrate_5Kbps[5]={ CAN_5Kbps,PRSEG_6TQ,PHSEG1_7TQ,PHSEG2_2TQ,SJW_1TQ};
uint8 code bitrate_10Kbps[5]={ CAN_10Kbps,PRSEG_6TQ,PHSEG1_7TQ,PHSEG2_2TQ,SJW_1TQ};
uint8 code bitrate_25Kbps[5]={ CAN_25Kbps,PRSEG_6TQ,PHSEG1_7TQ,PHSEG2_2TQ,SJW_1TQ};
uint8 code bitrate_50Kbps[5]={CAN_50Kbps,PRSEG_6TQ,PHSEG1_7TQ,PHSEG2_2TQ,SJW_1TQ};
uint8 code bitrate_100Kbps[5]={CAN_100Kbps,PRSEG_8TQ,PHSEG1_8TQ,PHSEG2_3TQ,SJW_1TQ};
uint8 code bitrate_125Kbps[5]={CAN_125Kbps,PRSEG_6TQ,PHSEG1_7TQ,PHSEG2_2TQ,SJW_1TQ};
uint8 code bitrate_250Kbps[5]={CAN_250Kbps,PRSEG_6TQ,PHSEG1_7TQ,PHSEG2_2TQ,SJW_1TQ};
uint8 code bitrate_500Kbps[5]={CAN_500Kbps,PRSEG_2TQ,PHSEG1_3TQ,PHSEG2_2TQ,SJW_1TQ};

/*******************************************************************************
* ������  : Exint_Init
* ����    : �ⲿ�ж�1��ʼ������
* ����    : ��
* ���    : ��
* ����ֵ  : ��
* ˵��    : ��
*******************************************************************************/
void Exint_Init(void) {
    PX1 = 1;        //�����ⲿ�ж�1���ж����ȼ�Ϊ�����ȼ�
    IT1 = 1;    //����INT1���ж����� (1:���½��� 0:�����غ��½���)
    EX1 = 1;    //ʹ��INT1�ж�
    EA = 1;    //ʹ�����ж�
}

/*******************************************************************************
* ������  : Exint_ISR
* ����    : �ⲿ�ж�1�жϷ����� ��Ƭ������P3.3��MCP2515 INT����
* ����    : ��
* ���    : ��
* ����ֵ  : ��
* ˵��    : ���ڼ��MCP2515�ж����ŵ��ж��ź�
*******************************************************************************/
void Exint_ISR(void) interrupt 2 using 1
{
    uint8 Flag;                                //CAN���յ����ݱ�־
    Flag = MCP2515_ReadByte(CANINTF);

    if (Flag&0x80) CAN_MERRF_Flag = 1;                            //CAN���Ĵ����жϱ�־λ
    if (Flag&0x40) CAN_WAKIF_Flag = 1;                            //CAN�����жϱ�־λ
    if (Flag&0x20) CAN_ERRIF_Flag = 1;                            //CAN�����жϱ�־λ��EFLG �Ĵ������ж���ж�Դ��
    if (Flag&0x10) CAN_TX2IF_Flag = 1;                            //MCP2515���ͻ�����2 ���жϱ�־λ
    if (Flag&0x08) CAN_TX1IF_Flag = 1;                            //MCP2515���ͻ�����1 ���жϱ�־λ
    if (Flag&0x04) CAN_TX0IF_Flag = 1;                            //MCP2515���ͻ�����0 ���жϱ�־λ
    if (Flag&0x02) CAN_RX1IF_Flag = 1;                            //MCP2515���ջ�����1 ���жϱ�־λ
    if (Flag&0x01) CAN_RX0IF_Flag = 1;                           //MCP2515���ջ�����0 ���жϱ�־λ
}

uint8 *NumToStr(uint16 num, uint8 radix) {
    static char str[8];      //����Ϊstatic������������ȫ�ֱ���

    uint8 tmp;
    uint8 i = 0;
    uint8 j = 0;
    uint8 NewStr[8] = {0};

    do      //�Ӹ�λ��ʼ��Ϊ�ַ���ֱ�����λ�����Ӧ�÷�ת
    {
        tmp = num % radix;
        num = num / radix;
        NewStr[i++] = tmp;
    } while (num > 0);
    do      //�Ӹ�λ��ʼ��Ϊ�ַ���ֱ�����λ�����Ӧ�÷�ת
    {
        tmp = NewStr[--i];
        if (tmp <= 9)              // ת��Ϊ 0-9 �� A-F
            str[j++] = tmp + '0';
        else
            str[j++] = tmp - 10 + 'A';
    } while (i > 0);
    str[j] = '\0';                 // ����ַ���������
    return str;
}

uint8 i;
/* ����Ҫ���͵����� ת����uart */
void Send(uint16 ID, uint8 EXIDE, uint8 DLC, uint8 *Send_data) {
    if (EXIDE)
    {
        printf("Can send ID: %08X,  DLC:%bx,  Data: ", ID, DLC);
    }
    else
    {
        printf("Can send ID: %8X,  DLC:%bx,  Data: ", ID, DLC);
    }

    for( i=0;i<DLC;i++ )
    {
        printf("%02bX " , Send_data[i]);
    }
    printf("\r\n");
    CAN_Send_buffer(ID, EXIDE, DLC, Send_data);
}

/* ����Ҫ���͵����� ת����uart, CAN_RX_Buf[14]*/
void Receive(uint8 RXB_CTRL_Address, uint8 *CAN_RX_Buf) {
    uint8 i;
    uint8 Receive_DLC = 0;
    uint8 Read_RXB_CTRL = 0;
    uint8 Receive_data[8] = {0};

    CAN_Receive_Buffer(RXB_CTRL_Address, CAN_RX_Buf);//CAN����һ֡����

    Receive_DLC = CAN_RX_Buf[5] & 0x0F; //��ȡ���յ������ݳ���
    printf("Receive RXB_CTRL_Address: %bX, DLC:%bx, Data:", RXB_CTRL_Address, Receive_DLC);

    for (i = 0; i < Receive_DLC; i++) //��ȡ���յ�������
    {
        Receive_data[i] = CAN_RX_Buf[6 + i];
        printf("%02bX " , Receive_data[i]);
    }
    printf("\r\n");
//  ��ȡ���ջ������������˲����� ���Ż�
    Read_RXB_CTRL = MCP2515_ReadByte(RXB_CTRL_Address);
    printf("Read_RXB_CTRL: %02bX,  RXF:%bX \r\n", Read_RXB_CTRL, Read_RXB_CTRL & 0x07);
}

/*******************************************************************************
* ������  : main
* ����    : ���������û������main������ʼ����
* ����    : ��
* ���    : ��
* ����ֵ  : ��
* ˵��    : ��
*******************************************************************************/
void main(void) {
    uint16 j;
    uint32 ID = 0x7FD;
    uint8 EXIDE = 0;
    uint8 DLC = 8;
    uint8 Send_data[] = {0x20, 0xF1, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

    UART_init();    //UART1��ʼ������
    Exint_Init();            //�ⲿ�ж�1��ʼ������

    MCP2515_Init(bitrate_100Kbps);

    for (j = 0; j < 2; j++) //�����ַ�����ֱ������0�Ž���
    {
        Send(ID, EXIDE, DLC, Send_data);
        ID++;
        EXIDE = !EXIDE;
        DLC--;
        Delay_Nms(1000);
    }

    Delay_Nms(2000);

    while (1) {

//        if (CAN_RX0IF_Flag == 1)                            //���ջ�����0 ���жϱ�־λ
//        {
//            CAN_RX0IF_Flag = 0;//CAN���յ����ݱ�־
//            Receive(RXB0CTRL, RXB_Value);//CAN����һ֡����
//            Delay_Nms(2000);  //�ƶ�����һ���ַ�
//
//        }
//        if (CAN_RX1IF_Flag == 1)                            //���ջ�����1 ���жϱ�־λ
//        {
//            CAN_RX1IF_Flag = 0;//CAN���յ����ݱ�־
//            CAN_Receive_Buffer(RXB1CTRL, RXB_Value);//CAN����һ֡����
////            UART_send_buffer(RXB_Value, 14); //����һ���ַ�
//            Delay_Nms(2000);  //�ƶ�����һ���ַ�
////			UART_send_buffer(RXB_Value,14); //����һ���ַ�
//        }
//
//        Delay_Nms(2000);
    }

}