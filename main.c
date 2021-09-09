#include <reg52.h>
#include "MCP2515.H"
#include <stdio.h>


//��������
extern void UART_init(void);
extern void UART_send_str(unsigned char d);
extern void UART_send_buffer(unsigned char *buffer,unsigned int len);

extern void Delay_Nms(unsigned int x);

extern unsigned char MCP2515_ReadByte(unsigned char addr);
extern void MCP2515_Init(unsigned char *CAN_Bitrate);
extern void CAN_Send_buffer(unsigned long int ID,unsigned char EXIDE,unsigned char DLC,unsigned char *Send_data);
extern void CAN_Receive_Buffer(unsigned char RXB_CTRL_Address,unsigned char *CAN_RX_Buf);


bit CAN_MERRF_Flag = 0;                            //CAN���Ĵ����жϱ�־λ
bit CAN_WAKIF_Flag = 0;                            //CAN�����жϱ�־λ
bit CAN_ERRIF_Flag = 0;                            //CAN�����жϱ�־λ��EFLG �Ĵ������ж���ж�Դ��
bit CAN_TX2IF_Flag = 0;                            //MCP2515���ͻ�����2 ���жϱ�־λ
bit CAN_TX1IF_Flag = 0;                            //MCP2515���ͻ�����1 ���жϱ�־λ
bit CAN_TX0IF_Flag = 0;                            //MCP2515���ͻ�����0 ���жϱ�־λ
bit CAN_RX1IF_Flag = 0;                            //MCP2515���ջ�����1 ���жϱ�־λ
bit CAN_RX0IF_Flag = 0;                            //MCP2515���ջ�����0 ���жϱ�־λ



char putchar(char c)  //printf���������putchar()
{
    UART_send_str(c);
    return c;
}

unsigned char CAN_R_Buffer[8];                        //CAN�������ݱ��滺����

//MCP2515������	Ҫ����FOSC=8M BRP=0..64 PRSEG=1..8 PHSEG1=3..16 PHSEG2=2..8 SJW=1..4
unsigned char code
bitrate_5Kbps[]={
        CAN_5Kbps,PRSEG_6TQ,PHSEG1_7TQ,PHSEG2_2TQ,SJW_1TQ};
unsigned char code
bitrate_10Kbps[]={
        CAN_10Kbps,PRSEG_6TQ,PHSEG1_7TQ,PHSEG2_2TQ,SJW_1TQ};
unsigned char code
bitrate_25Kbps[]={
        CAN_25Kbps,PRSEG_6TQ,PHSEG1_7TQ,PHSEG2_2TQ,SJW_1TQ};
unsigned char code
bitrate_50Kbps[]={
        CAN_50Kbps,PRSEG_6TQ,PHSEG1_7TQ,PHSEG2_2TQ,SJW_1TQ};
unsigned char code
bitrate_100Kbps[]={
        CAN_100Kbps,PRSEG_8TQ,PHSEG1_8TQ,PHSEG2_3TQ,SJW_1TQ};
unsigned char code
bitrate_125Kbps[]={
        CAN_125Kbps,PRSEG_6TQ,PHSEG1_7TQ,PHSEG2_2TQ,SJW_1TQ};
unsigned char code
bitrate_250Kbps[]={
        CAN_250Kbps,PRSEG_6TQ,PHSEG1_7TQ,PHSEG2_2TQ,SJW_1TQ};
unsigned char code
bitrate_500Kbps[]={
        CAN_500Kbps,PRSEG_2TQ,PHSEG1_3TQ,PHSEG2_2TQ,SJW_1TQ};

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
sbit P1_0 = P1 ^ 0;

void Exint_ISR(void)

interrupt 2 using 1
{
unsigned char Flag;                                //CAN���յ����ݱ�־
Flag = MCP2515_ReadByte(CANINTF);
//P1_0=!P1_0;

if (Flag&0x80)
CAN_MERRF_Flag = 1;                            //CAN���Ĵ����жϱ�־λ
if (Flag&0x40)
CAN_WAKIF_Flag = 1;                            //CAN�����жϱ�־λ
if (Flag&0x20)
CAN_ERRIF_Flag = 1;                            //CAN�����жϱ�־λ��EFLG �Ĵ������ж���ж�Դ��
if (Flag&0x10)
CAN_TX2IF_Flag = 1;                            //MCP2515���ͻ�����2 ���жϱ�־λ
if (Flag&0x08)
CAN_TX1IF_Flag = 1;                            //MCP2515���ͻ�����1 ���жϱ�־λ
if (Flag&0x04)
CAN_TX0IF_Flag = 1;                            //MCP2515���ͻ�����0 ���жϱ�־λ
if (Flag&0x02)
//P1_0=!P1_0;
CAN_RX1IF_Flag = 1;                            //MCP2515���ջ�����1 ���жϱ�־λ
if (Flag&0x01)
P1_0 = !P1_0;
CAN_RX0IF_Flag = 1;                            //MCP2515���ջ�����0 ���жϱ�־λ
}

/* ��һ���ڴ�����ת��Ϊʮ�������ַ��������� str ���ַ���ָ�룬���� src ��Դ���ݵ�ַ������ len �����ݳ��� */
void MemToStr(unsigned char *str, unsigned char *src, unsigned char len) {
    unsigned char tmp;
    while (len--) {
        tmp = *src >> 4;           // ȡ���� 4 λ
        if (tmp <= 9)              // ת��Ϊ 0-9 �� A-F
            *str++ = tmp + '0';
        else
            *str++ = tmp - 10 + 'A';
        tmp = *src & 0x0F;         // ȡ���� 4 λ
        if (tmp <= 9)              // ת��Ϊ 0-9 �� A-F
            *str++ = tmp + '0';
        else
            *str++ = tmp - 10 + 'A';
        *str++ = ' ';              // ת���� 1 ���ֽھ���� 1 ���ո�
        src++;
    }
    *str = '\0';                 // ����ַ���������
}

/* ��һ���ڴ�����ת��Ϊʮ�������ַ��������� str ���ַ���ָ�룬���� src ��Դ���ݵ�ַ������ len �����ݳ��� */
unsigned char *StrData(unsigned char *src, unsigned char len) {
    static char str[41];      //����Ϊstatic������������ȫ�ֱ��� ������ʱ�����14λ��14*3-1
    unsigned char i = 0;
    unsigned char tmp;
    while (len--) {
        tmp = *src >> 4;           // ȡ���� 4 λ
        if (tmp <= 9)              // ת��Ϊ 0-9 �� A-F
            str[i++] = tmp + '0';
        else
            str[i++] = tmp - 10 + 'A';
        tmp = *src & 0x0F;         // ȡ���� 4 λ
        if (tmp <= 9)              // ת��Ϊ 0-9 �� A-F
            str[i++] = tmp + '0';
        else
            str[i++] = tmp - 10 + 'A';
        str[i++] = ' ';              // ת���� 1 ���ֽھ���� 1 ���ո�
        src++;
    }
    str[--i] = '\0';                 // ����ַ���������
    return str;
}

unsigned char *NumToStr(unsigned int num, unsigned char radix) {
    static char str[8];      //����Ϊstatic������������ȫ�ֱ���

    unsigned char tmp;
    unsigned char i = 0;
    unsigned char j = 0;
    unsigned char NewStr[8] = {0};

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

unsigned char *StrLen(unsigned char *str) {
    unsigned char i = 0;
    while (*str++ != '\0') i++;
    return i;
}

unsigned char *StrID;
unsigned char *SendData;
unsigned char i;
/* ����Ҫ���͵����� ת����uart */
void Send(unsigned int ID, unsigned char EXIDE, unsigned char DLC, unsigned char *Send_data) {
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
void Receive(unsigned char RXB_CTRL_Address, unsigned char *CAN_RX_Buf) {
    unsigned char i;
    unsigned char Receive_DLC = 0;
    unsigned char Read_RXB_CTRL = 0;
    unsigned char Receive_data[8] = {0};

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
//unsigned char TXB_Value[]={0x0,0x1,0x2,0x3,0x04,0x05,0x06,0x27,0x08,0x09,0x10,0x11,0x12,0x13};
unsigned char RXB_Value[14] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D};

unsigned char Read_Value[] = {0x0, 0x1, 0x2, 0x3, 0x04, 0x05, 0x06, 0x27};

//unsigned char ini00[]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0,0x00,0x00,0x00,0x00,0x00,0x00};
void main(void) {
    unsigned int j;
    unsigned long int ID = 0x7FD;
    unsigned char EXIDE = 0;
    unsigned char DLC = 8;
    unsigned char Send_data[] = {0x20, 0xF1, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

    UART_init();    //UART1��ʼ������
    Exint_Init();            //�ⲿ�ж�1��ʼ������
    SendData = StrData(Send_data, DLC);
    printf("123123Data:%s \r\n", SendData);

    MCP2515_Init(bitrate_100Kbps);

    for (j = 0; j < 2; j++) //�����ַ�����ֱ������0�Ž���
    {
        Send(ID, EXIDE, DLC, Send_data);
//        CAN_Send_buffer(ID,EXIDE,DLC,Send_data);
        ID++;
        EXIDE = !EXIDE;
        DLC--;
        Delay_Nms(1000);
    }

    Delay_Nms(2000);

    while (1) {

        if (CAN_RX0IF_Flag == 1)                            //���ջ�����0 ���жϱ�־λ
        {
            CAN_RX0IF_Flag = 0;//CAN���յ����ݱ�־
            Receive(RXB0CTRL, RXB_Value);//CAN����һ֡����
            Delay_Nms(2000);  //�ƶ�����һ���ַ�

        }
        if (CAN_RX1IF_Flag == 1)                            //���ջ�����1 ���жϱ�־λ
        {
            CAN_RX1IF_Flag = 0;//CAN���յ����ݱ�־
            CAN_Receive_Buffer(RXB1CTRL, RXB_Value);//CAN����һ֡����
//            UART_send_buffer(RXB_Value, 14); //����һ���ַ�
            Delay_Nms(2000);  //�ƶ�����һ���ַ�
//			UART_send_buffer(RXB_Value,14); //����һ���ַ�
        }

        Delay_Nms(2000);
    }

}