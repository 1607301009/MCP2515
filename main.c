#include <reg52.h>
#include "MCP2515.H"
#include <stdio.h>


//声明函数
extern void UART_init(void);
extern void UART_send_str(unsigned char d);
extern void UART_send_buffer(unsigned char *buffer,unsigned int len);

extern void Delay_Nms(unsigned int x);

extern unsigned char MCP2515_ReadByte(unsigned char addr);
extern void MCP2515_Init(unsigned char *CAN_Bitrate);
extern void CAN_Send_buffer(unsigned long int ID,unsigned char EXIDE,unsigned char DLC,unsigned char *Send_data);
extern void CAN_Receive_Buffer(unsigned char RXB_CTRL_Address,unsigned char *CAN_RX_Buf);


bit CAN_MERRF_Flag = 0;                            //CAN报文错误中断标志位
bit CAN_WAKIF_Flag = 0;                            //CAN唤醒中断标志位
bit CAN_ERRIF_Flag = 0;                            //CAN错误中断标志位（EFLG 寄存器中有多个中断源）
bit CAN_TX2IF_Flag = 0;                            //MCP2515发送缓冲器2 空中断标志位
bit CAN_TX1IF_Flag = 0;                            //MCP2515发送缓冲器1 空中断标志位
bit CAN_TX0IF_Flag = 0;                            //MCP2515发送缓冲器0 空中断标志位
bit CAN_RX1IF_Flag = 0;                            //MCP2515接收缓冲器1 满中断标志位
bit CAN_RX0IF_Flag = 0;                            //MCP2515接收缓冲器0 满中断标志位



char putchar(char c)  //printf函数会调用putchar()
{
    UART_send_str(c);
    return c;
}

unsigned char CAN_R_Buffer[8];                        //CAN接收数据保存缓冲区

//MCP2515波特率	要考虑FOSC=8M BRP=0..64 PRSEG=1..8 PHSEG1=3..16 PHSEG2=2..8 SJW=1..4
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
* 函数名  : Exint_Init
* 描述    : 外部中断1初始化函数
* 输入    : 无
* 输出    : 无
* 返回值  : 无
* 说明    : 无
*******************************************************************************/
void Exint_Init(void) {
    PX1 = 1;        //设置外部中断1的中断优先级为高优先级
    IT1 = 1;    //设置INT1的中断类型 (1:仅下降沿 0:上升沿和下降沿)
    EX1 = 1;    //使能INT1中断
    EA = 1;    //使能总中断
}

/*******************************************************************************
* 函数名  : Exint_ISR
* 描述    : 外部中断1中断服务函数 单片机引脚P3.3接MCP2515 INT引脚
* 输入    : 无
* 输出    : 无
* 返回值  : 无
* 说明    : 用于检测MCP2515中断引脚的中断信号
*******************************************************************************/
sbit P1_0 = P1 ^ 0;

void Exint_ISR(void)

interrupt 2 using 1
{
unsigned char Flag;                                //CAN接收到数据标志
Flag = MCP2515_ReadByte(CANINTF);
//P1_0=!P1_0;

if (Flag&0x80)
CAN_MERRF_Flag = 1;                            //CAN报文错误中断标志位
if (Flag&0x40)
CAN_WAKIF_Flag = 1;                            //CAN唤醒中断标志位
if (Flag&0x20)
CAN_ERRIF_Flag = 1;                            //CAN错误中断标志位（EFLG 寄存器中有多个中断源）
if (Flag&0x10)
CAN_TX2IF_Flag = 1;                            //MCP2515发送缓冲器2 空中断标志位
if (Flag&0x08)
CAN_TX1IF_Flag = 1;                            //MCP2515发送缓冲器1 空中断标志位
if (Flag&0x04)
CAN_TX0IF_Flag = 1;                            //MCP2515发送缓冲器0 空中断标志位
if (Flag&0x02)
//P1_0=!P1_0;
CAN_RX1IF_Flag = 1;                            //MCP2515接收缓冲器1 满中断标志位
if (Flag&0x01)
P1_0 = !P1_0;
CAN_RX0IF_Flag = 1;                            //MCP2515接收缓冲器0 满中断标志位
}

/* 将一段内存数据转换为十六进制字符串，参数 str 是字符串指针，参数 src 是源数据地址，参数 len 是数据长度 */
void MemToStr(unsigned char *str, unsigned char *src, unsigned char len) {
    unsigned char tmp;
    while (len--) {
        tmp = *src >> 4;           // 取出高 4 位
        if (tmp <= 9)              // 转换为 0-9 或 A-F
            *str++ = tmp + '0';
        else
            *str++ = tmp - 10 + 'A';
        tmp = *src & 0x0F;         // 取出低 4 位
        if (tmp <= 9)              // 转换为 0-9 或 A-F
            *str++ = tmp + '0';
        else
            *str++ = tmp - 10 + 'A';
        *str++ = ' ';              // 转换完 1 个字节就添加 1 个空格
        src++;
    }
    *str = '\0';                 // 添加字符串结束符
}

/* 将一段内存数据转换为十六进制字符串，参数 str 是字符串指针，参数 src 是源数据地址，参数 len 是数据长度 */
unsigned char *StrData(unsigned char *src, unsigned char len) {
    static char str[41];      //必须为static变量，或者是全局变量 读数据时最多有14位，14*3-1
    unsigned char i = 0;
    unsigned char tmp;
    while (len--) {
        tmp = *src >> 4;           // 取出高 4 位
        if (tmp <= 9)              // 转换为 0-9 或 A-F
            str[i++] = tmp + '0';
        else
            str[i++] = tmp - 10 + 'A';
        tmp = *src & 0x0F;         // 取出低 4 位
        if (tmp <= 9)              // 转换为 0-9 或 A-F
            str[i++] = tmp + '0';
        else
            str[i++] = tmp - 10 + 'A';
        str[i++] = ' ';              // 转换完 1 个字节就添加 1 个空格
        src++;
    }
    str[--i] = '\0';                 // 添加字符串结束符
    return str;
}

unsigned char *NumToStr(unsigned int num, unsigned char radix) {
    static char str[8];      //必须为static变量，或者是全局变量

    unsigned char tmp;
    unsigned char i = 0;
    unsigned char j = 0;
    unsigned char NewStr[8] = {0};

    do      //从各位开始变为字符，直到最高位，最后应该反转
    {
        tmp = num % radix;
        num = num / radix;
        NewStr[i++] = tmp;
    } while (num > 0);
    do      //从各位开始变为字符，直到最高位，最后应该反转
    {
        tmp = NewStr[--i];
        if (tmp <= 9)              // 转换为 0-9 或 A-F
            str[j++] = tmp + '0';
        else
            str[j++] = tmp - 10 + 'A';
    } while (i > 0);
    str[j] = '\0';                 // 添加字符串结束符
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
/* 将需要发送的数据 转发到uart */
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

/* 将需要发送的数据 转发到uart, CAN_RX_Buf[14]*/
void Receive(unsigned char RXB_CTRL_Address, unsigned char *CAN_RX_Buf) {
    unsigned char i;
    unsigned char Receive_DLC = 0;
    unsigned char Read_RXB_CTRL = 0;
    unsigned char Receive_data[8] = {0};

    CAN_Receive_Buffer(RXB_CTRL_Address, CAN_RX_Buf);//CAN接收一帧数据

    Receive_DLC = CAN_RX_Buf[5] & 0x0F; //获取接收到的数据长度
    printf("Receive RXB_CTRL_Address: %bX, DLC:%bx, Data:", RXB_CTRL_Address, Receive_DLC);

    for (i = 0; i < Receive_DLC; i++) //获取接收到的数据
    {
        Receive_data[i] = CAN_RX_Buf[6 + i];
        printf("%02bX " , Receive_data[i]);
    }
    printf("\r\n");
//  获取接收缓存器及验收滤波器， 待优化
    Read_RXB_CTRL = MCP2515_ReadByte(RXB_CTRL_Address);
    printf("Read_RXB_CTRL: %02bX,  RXF:%bX \r\n", Read_RXB_CTRL, Read_RXB_CTRL & 0x07);
}

/*******************************************************************************
* 函数名  : main
* 描述    : 主函数，用户程序从main函数开始运行
* 输入    : 无
* 输出    : 无
* 返回值  : 无
* 说明    : 无
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

    UART_init();    //UART1初始化配置
    Exint_Init();            //外部中断1初始化函数
    SendData = StrData(Send_data, DLC);
    printf("123123Data:%s \r\n", SendData);

    MCP2515_Init(bitrate_100Kbps);

    for (j = 0; j < 2; j++) //发送字符串，直到遇到0才结束
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

        if (CAN_RX0IF_Flag == 1)                            //接收缓冲器0 满中断标志位
        {
            CAN_RX0IF_Flag = 0;//CAN接收到数据标志
            Receive(RXB0CTRL, RXB_Value);//CAN接收一帧数据
            Delay_Nms(2000);  //移动到下一个字符

        }
        if (CAN_RX1IF_Flag == 1)                            //接收缓冲器1 满中断标志位
        {
            CAN_RX1IF_Flag = 0;//CAN接收到数据标志
            CAN_Receive_Buffer(RXB1CTRL, RXB_Value);//CAN接收一帧数据
//            UART_send_buffer(RXB_Value, 14); //发送一个字符
            Delay_Nms(2000);  //移动到下一个字符
//			UART_send_buffer(RXB_Value,14); //发送一个字符
        }

        Delay_Nms(2000);
    }

}