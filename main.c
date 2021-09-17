#include <reg52.h>
#include "MCP2515.H"
#include <stdio.h>


//声明函数
extern void UART_init(void);
extern void UART_send_str(uint8 d);
extern void UART_send_buffer(uint8 *buffer,uint16 len);

extern void Delay_Nms(uint16 x);

extern uint8 MCP2515_ReadByte(uint8 addr);
extern void MCP2515_WriteByte(uint8 addr,uint8 dat);
extern void MCP2515_Init(uint8 *CAN_Bitrate);
extern void CAN_Send_buffer(uint32 ID,uint8 EXIDE,uint8 DLC,uint8 *Send_data);
extern void CAN_Receive_Buffer(uint8 RXB_CTRL_Address,uint8 *CAN_RX_Buf);


bool CAN_MERRF_Flag = 0;                            //CAN报文错误中断标志位
bool CAN_WAKIF_Flag = 0;                            //CAN唤醒中断标志位
bool CAN_ERRIF_Flag = 0;                            //CAN错误中断标志位（EFLG 寄存器中有多个中断源）
bool CAN_TX2IF_Flag = 0;                            //MCP2515发送缓冲器2 空中断标志位
bool CAN_TX1IF_Flag = 0;                            //MCP2515发送缓冲器1 空中断标志位
bool CAN_TX0IF_Flag = 0;                            //MCP2515发送缓冲器0 空中断标志位
bool CAN_RX1IF_Flag = 0;                            //MCP2515接收缓冲器1 满中断标志位
bool CAN_RX0IF_Flag = 0;                            //MCP2515接收缓冲器0 满中断标志位



char putchar(char c)  //printf函数会调用putchar()
{
    UART_send_str(c);
    return c;
}

//MCP2515波特率	要考虑FOSC=8M BRP=0..64 PRSEG=1..8 PHSEG1=3..16 PHSEG2=2..8 SJW=1..4
uint8 code bitrate_5Kbps[5]={ CAN_5Kbps,PRSEG_6TQ,PHSEG1_7TQ,PHSEG2_2TQ,SJW_1TQ};
uint8 code bitrate_10Kbps[5]={ CAN_10Kbps,PRSEG_6TQ,PHSEG1_7TQ,PHSEG2_2TQ,SJW_1TQ};
uint8 code bitrate_25Kbps[5]={ CAN_25Kbps,PRSEG_6TQ,PHSEG1_7TQ,PHSEG2_2TQ,SJW_1TQ};
uint8 code bitrate_50Kbps[5]={CAN_50Kbps,PRSEG_6TQ,PHSEG1_7TQ,PHSEG2_2TQ,SJW_1TQ};
uint8 code bitrate_100Kbps[5]={CAN_100Kbps,PRSEG_8TQ,PHSEG1_8TQ,PHSEG2_3TQ,SJW_1TQ};
uint8 code bitrate_125Kbps[5]={CAN_125Kbps,PRSEG_6TQ,PHSEG1_7TQ,PHSEG2_2TQ,SJW_1TQ};
uint8 code bitrate_250Kbps[5]={CAN_250Kbps,PRSEG_6TQ,PHSEG1_7TQ,PHSEG2_2TQ,SJW_1TQ};
uint8 code bitrate_500Kbps[5]={CAN_500Kbps,PRSEG_2TQ,PHSEG1_3TQ,PHSEG2_2TQ,SJW_1TQ};

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
void Exint_ISR(void) interrupt 2 using 1
{
    uint8 Flag;                                //CAN接收到数据标志
    Flag = MCP2515_ReadByte(CANINTF);

    if (Flag&0x80) CAN_MERRF_Flag = 1;                            //CAN报文错误中断标志位
    if (Flag&0x40) CAN_WAKIF_Flag = 1;                            //CAN唤醒中断标志位
    if (Flag&0x20) CAN_ERRIF_Flag = 1;                            //CAN错误中断标志位（EFLG 寄存器中有多个中断源）
    if (Flag&0x10) CAN_TX2IF_Flag = 1;                            //MCP2515发送缓冲器2 空中断标志位
    if (Flag&0x08) CAN_TX1IF_Flag = 1;                            //MCP2515发送缓冲器1 空中断标志位
    if (Flag&0x04) CAN_TX0IF_Flag = 1;                            //MCP2515发送缓冲器0 空中断标志位
    if (Flag&0x02) CAN_RX1IF_Flag = 1;                            //MCP2515接收缓冲器1 满中断标志位
    if (Flag&0x01) CAN_RX0IF_Flag = 1;                           //MCP2515接收缓冲器0 满中断标志位
}

uint8 *NumToStr(uint16 num, uint8 radix) {
    static char str[8];      //必须为static变量，或者是全局变量

    uint8 tmp;
    uint8 i = 0;
    uint8 j = 0;
    uint8 NewStr[8] = {0};

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

uint8 i;
///* 将需要发送的数据 转发到uart */
//void Send(uint16 ID, uint8 EXIDE, uint8 DLC, uint8 *Send_data) {
//    if (EXIDE)
//    {
//        printf("Can send ID: %08X,  DLC:%bx,  Data: ", ID, DLC);
//    }
//    else
//    {
//        printf("Can send ID: %8X,  DLC:%bx,  Data: ", ID, DLC);
//    }
//
//    for( i=0;i<DLC;i++ )
//    {
//        printf("%02bX " , Send_data[i]);
//    }
//    printf("\r\n");
//    CAN_Send_buffer(ID, EXIDE, DLC, Send_data);
//}

void ShowMsg(MsgStruct *Msg)
{
    uint8 i;
    uint32 ID = Msg->ID;
    uint8 EXIDE = Msg->EXIDE;
    uint8 DLC = Msg->DLC;

    if (Msg->IsSend)
    {
        printf("Can send    ");
    } else {
        printf("Can recevie ");
    }

    if (EXIDE)
    {
        printf("ID: %07lX,  DLC:%bx,  Data: ", ID, DLC);
    }
    else
    {
        printf("ID: %7lX,  DLC:%bx,  Data: ", ID, DLC);
    }

    for( i=0;i<DLC;i++ )
    {
        printf("%02bX " , Msg->DATA[i]);
    }

    printf("\r\n");
}

/* 将需要发送的数据 转发到uart */
void Send(MsgStruct *SendMsg) {
    uint32 ID = SendMsg->ID;
    uint8 EXIDE = SendMsg->EXIDE;
    uint8 DLC = SendMsg->DLC;
//    ShowMsg(SendMsg);
    CAN_Send_buffer(ID, EXIDE, DLC, SendMsg->DATA);
}

///* 将需要发送的数据 转发到uart, CAN_RX_Buf[14]*/
//void Receive(uint8 RXB_CTRL_Address, uint8 *CAN_RX_Buf) {
//    uint8 i;
//    uint8 Receive_DLC = 0;
//    uint8 Read_RXB_CTRL = 0;
//    uint8 Receive_data[8] = {0};
//
//    CAN_Receive_Buffer(RXB_CTRL_Address, CAN_RX_Buf);//CAN接收一帧数据
//
//    Receive_DLC = CAN_RX_Buf[5] & 0x0F; //获取接收到的数据长度
//    printf("Receive RXB_CTRL_Address: %bX, DLC:%bx, Data:", RXB_CTRL_Address, Receive_DLC);
//
//    for (i = 0; i < Receive_DLC; i++) //获取接收到的数据
//    {
//        Receive_data[i] = CAN_RX_Buf[6 + i];
//        printf("%02bX " , Receive_data[i]);
//    }
//    printf("\r\n");
////  获取接收缓存器及验收滤波器， 待优化
//    Read_RXB_CTRL = MCP2515_ReadByte(RXB_CTRL_Address);
//    printf("Read_RXB_CTRL: %02bX,  RXF:%bX \r\n", Read_RXB_CTRL, Read_RXB_CTRL & 0x07);
//}
//
 /* 将需要发送的数据 转发到uart, CAN_RX_Buf[14]*/
 void Receive(uint8 RXB_CTRL_Address, MsgStruct *RecMsg) {
     uint8 i;

     uint8 RXBnCTRL = MCP2515_ReadByte(RXB_CTRL_Address);
     uint8 RXBnSIDH = MCP2515_ReadByte(RXB_CTRL_Address + 1);
     uint8 RXBnSIDL = MCP2515_ReadByte(RXB_CTRL_Address + 2);
     uint8 RXBnEID8 = MCP2515_ReadByte(RXB_CTRL_Address + 3);
     uint8 RXBnEID0 = MCP2515_ReadByte(RXB_CTRL_Address + 4);
     uint8 RXBnDLC  = MCP2515_ReadByte(RXB_CTRL_Address + 5);

     RecMsg->EXIDE = (RXBnSIDL & 0x8) >> 3;  // 扩展标识符标志位 1 = 收到的报文是扩展帧, 0 = 收到的报文是标准帧
     RecMsg->DLC = RXBnDLC & 0x0F;

     if (RecMsg->EXIDE)
     {
         uint32 SID = (RXBnSIDH<<3) | (RXBnSIDL>>5);
         uint32 EID = (RXBnSIDL & 3) << 16 | (RXBnEID8<<8) | RXBnEID0;
         RecMsg->ID = SID<<18 | EID;
     }
     else
     {
         uint32 SID = (RXBnSIDH<<3) | (RXBnSIDL>>5);
         RecMsg->ID = SID;
     }

     for (i = 0; i < RecMsg->DLC; i++) //获取接收到的数据
     {
        RecMsg->DATA[i] = MCP2515_ReadByte(RXB_CTRL_Address + 6 + i);
     }

//     if (RXB_CTRL_Address==RXB0CTRL)
//     {
//         MCP2515_WriteByte(CANINTF, MCP2515_ReadByte(CANINTF) | 0xFE);//清除中断标志位(中断标志寄存器必须由MCU清零)
        MCP2515_WriteByte(CANINTF, 0);//清除中断标志位(中断标志寄存器必须由MCU清零)
//     }
//     else if (RXB_CTRL_Address==RXB1CTRL)
//     {
//         MCP2515_WriteByte(CANINTF, MCP2515_ReadByte(CANINTF) | 0xFD);//清除中断标志位(中断标志寄存器必须由MCU清零)
//     }
}



/*******************************************************************************
* 函数名  : main
* 描述    : 主函数，用户程序从main函数开始运行
* 输入    : 无
* 输出    : 无
* 返回值  : 无
* 说明    : 无
*******************************************************************************/
void main(void) {
    uint32 ID = 0x7FE;
    uint8 EXIDE = 0;
    uint8 DLC = 8;
    uint8 i;
    uint8 Send_data[] = {0x20, 0xF1, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

    MsgStruct SendMsg;
    MsgStruct RecMsg;

    UART_init();    //UART1初始化配置
    Exint_Init();            //外部中断1初始化函数
    MCP2515_Init(bitrate_100Kbps);

    SendMsg.IsSend = 0x1;

    SendMsg.ID = ID;
    SendMsg.TYPE = 0x2;
    SendMsg.EXIDE = 0x0;
    SendMsg.DLC = 8;
    *SendMsg.DATA = *Send_data;



    RecMsg.IsSend = 0;
//    Send(&SendMsg);

    for (i = 0; i < 2; i++) //发送字符串，直到遇到0才结束
    {
        Send(&SendMsg);
        ShowMsg(&SendMsg);
        SendMsg.ID = 0x100;
        SendMsg.TYPE = 0x2;
        SendMsg.EXIDE = 0x1;
        SendMsg.DLC = 7;

        Delay_Nms(3000);

        if (CAN_RX0IF_Flag == 1)                            //接收缓冲器0 满中断标志位
        {
            Receive(RXB0CTRL, &RecMsg);
            CAN_RX0IF_Flag = 0;
            ShowMsg(&RecMsg);
            printf("CAN_RX0IF_Flag = 0\r\n");
        }

        if (CAN_RX1IF_Flag == 1)                            //接收缓冲器1 满中断标志位
        {
            Receive(RXB1CTRL, &RecMsg);
            CAN_RX1IF_Flag = 0;
            ShowMsg(&RecMsg);
            printf("CAN_RX1IF_Flag = 1\r\n");
        }
    }

    Delay_Nms(2000);

    while (1) {

//        if (CAN_RX0IF_Flag == 1)                            //接收缓冲器0 满中断标志位
//        {
//            CAN_RX0IF_Flag = 0;//CAN接收到数据标志
//            Receive(RXB0CTRL, RXB_Value);//CAN接收一帧数据
//            Delay_Nms(2000);  //移动到下一个字符
//
//        }
//        if (CAN_RX1IF_Flag == 1)                            //接收缓冲器1 满中断标志位
//        {
//            CAN_RX1IF_Flag = 0;//CAN接收到数据标志
//            CAN_Receive_Buffer(RXB1CTRL, RXB_Value);//CAN接收一帧数据
////            UART_send_buffer(RXB_Value, 14); //发送一个字符
//            Delay_Nms(2000);  //移动到下一个字符
////			UART_send_buffer(RXB_Value,14); //发送一个字符
//        }
//
//        Delay_Nms(2000);
    }

}