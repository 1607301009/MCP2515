#include <reg52.h>
#include "MCP2515.H"
#include <stdio.h>


//声明函数
extern void UART_init(void);
extern void UART_send_str(uint8 d);
//extern void UART_send_buffer(uint8 *buffer,uint16 len);

extern void Delay_Nms(uint16 x);

extern uint8 MCP2515_ReadByte(uint8 addr);
extern void MCP2515_WriteByte(uint8 addr,uint8 dat);
//extern void MCP2515_Init(uint8 *CAN_Bitrate);
extern void Can_Init(CanCfgStruct *CanCfg);
extern void CAN_Send_buffer(uint32 ID,uint8 EXIDE,uint8 DLC,uint8 *Send_data);
//extern void CAN_Receive_Buffer(uint8 RXB_CTRL_Address,uint8 *CAN_RX_Buf);

// ID转化模块
extern uint32 Get_ID_For_Array(uint8 *array, uint8 offset);
extern void Set_Array_For_ID(uint8 *array, uint8 offset, uint32 ID, uint8 EXIDE);
extern uint32 Get_ID_For_Buf(uint8 buf_addr);
extern void Set_Buf_For_ID(uint8 buf_addr, uint32 ID, uint8 EXIDE);

extern void Set_Bitrate_Array(uint8 _5Kbps, uint8 *bitrate);

// 存储模块
extern void E2Read(unsigned char *buf, unsigned char addr, unsigned char len);
extern void E2Write(unsigned char *buf, unsigned char addr, unsigned char len);

//bool CAN_MERRF_Flag = 0;                            //CAN报文错误中断标志位
//bool CAN_WAKIF_Flag = 0;                            //CAN唤醒中断标志位
//bool CAN_ERRIF_Flag = 0;                            //CAN错误中断标志位（EFLG 寄存器中有多个中断源）
//bool CAN_TX2IF_Flag = 0;                            //MCP2515发送缓冲器2 空中断标志位
//bool CAN_TX1IF_Flag = 0;                            //MCP2515发送缓冲器1 空中断标志位
//bool CAN_TX0IF_Flag = 0;                            //MCP2515发送缓冲器0 空中断标志位
bool CAN_RX1IF_Flag = false;                        //MCP2515接收缓冲器1 满中断标志位
bool CAN_RX0IF_Flag = false;                        //MCP2515接收缓冲器0 满中断标志位

uint8 main_status = 1;

/*******************************************************************************
* 描述    : 重定向putchar, 将printf函数打印到串口中。
*******************************************************************************/
char putchar(char c)
{
    UART_send_str(c);
    return c;
}

/*******************************************************************************
* 函数名  : Exint_Init
* 描述    : 外部中断1初始化函数
*******************************************************************************/
void Exint_Init(void) {
    PX1 = 1;    //设置外部中断1的中断优先级为高优先级
    IT1 = 1;    //设置INT1的中断类型 (1:仅下降沿 0:上升沿和下降沿)
    EX1 = 1;    //使能INT1中断
    EA = 1;     //使能总中断
}

/*******************************************************************************
* 函数名  : Exint_ISR
* 描述    : 外部中断1中断服务函数 单片机引脚P3.3接MCP2515 INT引脚
* 说明    : 用于检测MCP2515中断引脚的中断信号
*******************************************************************************/
void Exint_ISR(void) interrupt 2 using 1
{
    uint8 Flag;                                //CAN接收到数据标志
    Flag = MCP2515_ReadByte(CANINTF);

//    if (Flag&0x80) CAN_MERRF_Flag = 1;                            //CAN报文错误中断标志位
//    if (Flag&0x40) CAN_WAKIF_Flag = 1;                            //CAN唤醒中断标志位
//    if (Flag&0x20) CAN_ERRIF_Flag = 1;                            //CAN错误中断标志位（EFLG 寄存器中有多个中断源）
//    if (Flag&0x10) CAN_TX2IF_Flag = 1;                            //MCP2515发送缓冲器2 空中断标志位
//    if (Flag&0x08) CAN_TX1IF_Flag = 1;                            //MCP2515发送缓冲器1 空中断标志位
//    if (Flag&0x04) CAN_TX0IF_Flag = 1;                            //MCP2515发送缓冲器0 空中断标志位
    if (Flag&0x02) CAN_RX1IF_Flag = true;                            //MCP2515接收缓冲器1 满中断标志位
    if (Flag&0x01) CAN_RX0IF_Flag = true;                           //MCP2515接收缓冲器0 满中断标志位
}

/*******************************************************************************
* 描述    : 将msg打印出来
* 输入    : Msg结构体
*******************************************************************************/
//void Printf_Msg(MsgStruct *Msg) {
//    uint8 i;
//    uint32 ID = Msg->ID;
//    uint8 EXIDE = Msg->EXIDE;
//    uint8 DLC = Msg->DLC;
//
//    if (Msg->IsSend) {
//        printf("send ");
//    } else {
//        printf("rec  ");
//    }
//
//    if (EXIDE) {
//        printf("ID: %07lX,  DLC:%bx,  Data: ", ID, DLC);
//    } else {
//        printf("ID: %7lX,  DLC:%bx,  Data: ", ID, DLC);
//    }
//
//    for (i = 0; i < DLC; i++) {
//        printf("%02bX ", Msg->DATA[i]);
//    }
//
//    printf("\r\n");
//}

/* 设置调试标志位，将需要发送的数据 转发到uart */
void Send(MsgStruct *SendMsg) {
    uint32 ID = SendMsg->ID;
    uint8 EXIDE = SendMsg->EXIDE;
    uint8 DLC = SendMsg->DLC | (SendMsg->RTR << 6);
    CAN_Send_buffer(ID, EXIDE, DLC, SendMsg->DATA);
}


/*******************************************************************************
* 描述    : 通过Can msg设置CAN配置,
* data[0]保留标志位，
* data[1]:addr, 2: data_len, [3:7]:data_arr
* 说明    : 设置完成后需下发初始化使能信号。
* 初步模块简单设置
*******************************************************************************/
void msg_set_E2(uint8 *msg_data) {
    uint8 i;
    uint8 addr = msg_data[1];
    uint8 len = msg_data[2];
    uint8 array[8];
    for (i = 0; i < len; i++) //发送字符串，直到遇到0才结束
        {
        array[i] = msg_data[i+3];
        }
    E2Write(array, addr, len + 3);
}

/*******************************************************************************
* 描述    : 接收到信息后,根据状态进行反应
* 输入    : msg
* 说明    : 设计盲应答模式， 统一一个ID，上报和调试，方便查找设备
*******************************************************************************/
//void action_rec_msg(MsgStruct *RecMsg) {
//    if (RecMsg->FILHIT == 0) {  // 滤波器0H 进行 配置信息
//        if (RecMsg->DATA[0] == action_status) {
//            // 设置主程序运行状态， 使能配置， 获取配置等
//            main_status =RecMsg->DATA[1];
//        } else if (RecMsg->DATA[0] == action_E2) {
//            // 从rec中设置E2， config, 波特率只能通过E2写 的方式修改
//            msg_set_E2(RecMsg->DATA);
//        } else if (RecMsg->DATA[0] == action_MCP2515){  // 直接设置MCP2515寄存器
//            // [0] 状态标志位， [1] addr, [2] data
//            MCP2515_WriteByte(RecMsg->DATA[1], RecMsg->DATA[2]);
//        }
//    } else if (RecMsg->FILHIT == 1) {  // 滤波器1H 读取MCP2515数据
//        if (RecMsg->DATA[0] == action_status) {
//            // 读取主程序运行状态
//            RecMsg->DLC = 1;
//            RecMsg->DATA[0] == main_status;
//        } else if (RecMsg->DATA[0] == action_E2) {
//            // 从rec中设置E2， config
//            RecMsg->DLC = RecMsg->DATA[2];
//            E2Read(RecMsg->DATA, RecMsg->DATA[1], RecMsg->DATA[2]);  // 从 EEPROM 读取一段数据
//            // 发送应答msg
//        } else if (RecMsg->DATA[0] == action_MCP2515) {  // 直接读取MCP2515寄存器
//            RecMsg->DLC = 1;
//            RecMsg->DATA[0] == MCP2515_ReadByte(RecMsg->DATA[1]);
//        }
//        if (RecMsg->RTR  == 0) {
//            // 如果是远程帧，说明是应答过的
//            RecMsg->RTR = 1; // 应答模式设计为远程帧， 解决回环模式重复发送问题
//            Send(&RecMsg);
//        }
//        Printf_Msg(&RecMsg);
//    } else if (RecMsg->FILHIT == 5) {  // 滤波器5H 盲应答模式
//        return;
//    } else {  // GPIO 设置
//        return;
//    }
//}
//
///* 将需要发送的数据 转发到uart, CAN_RX_Buf[14]*/
//void Receive(uint8 RXB_CTRL_Address, MsgStruct *RecMsg) {
//    uint8 i;
//
//    uint8 RXBnCTRL = MCP2515_ReadByte(RXB_CTRL_Address);
//    uint8 RXBnDLC = MCP2515_ReadByte(RXB_CTRL_Address + 5);
//    RecMsg->DLC = RXBnDLC & 0x0F;
//    RecMsg->RTR = RXBnDLC >> 6;
//
//    if (RXB_CTRL_Address == RXB0CTRL) {
//        RecMsg->FILHIT = RXBnCTRL & 0x3;
//    } else {
//        RecMsg->FILHIT = RXBnCTRL & 0x7;
//    }
//
//    RecMsg->ID = Get_ID_For_Buf(RXB_CTRL_Address + 1);
//    RecMsg->EXIDE = (MCP2515_ReadByte(RXB_CTRL_Address + 2) & 0x8) >> 3;
//
//    for (i = 0; i < RecMsg->DLC; i++) //获取接收到的数据
//    {
//        RecMsg->DATA[i] = MCP2515_ReadByte(RXB_CTRL_Address + 6 + i);
//    }
//    // 根据接收的msg, 进行动作
//    action_rec_msg(&RecMsg);
//}

void printE2Write(uint8 *E2_data, uint8 add, uint8 Len) {
    uint8 i;
    printf("add: %02bX Len: %02bX Data:", add, Len);
    for (i = 0; i < Len; i++) //发送字符串，直到遇到0才结束
        {
        printf(" %02bX", E2_data[i]);
        }
    printf("\r\n");
}

// 本地调试使用
void SaveCfgToE2(CanCfgStruct *CanCfg) {
    uint8 E2_data[8];
//    uint8 read_data[8];
printf("SaveCfgToE2  : %02bX \r\n", main_status);
    E2_data[E2_5Kbps] = CanCfg->_5Kbps;
    E2_data[E2_BUKT_enable] = CanCfg->BUKT_enable;
    E2_data[E2_RXB0RXM] = CanCfg->RXB0RXM;
    E2_data[E2_RXB1RXM] = CanCfg->RXB1RXM;
    E2_data[E2_CAN_MODE] = CanCfg->CAN_MODE;
    E2_data[E2_CANINTE_enable] = CanCfg->CANINTE_enable;
    E2_data[E2_CANINTF_enable] = CanCfg->CANINTF_enable;

    printf("SaveCfgToE2  : %02bX %02bX %02bX %02bX %02bX \r\n", E2_data[0], E2_data[1], E2_data[2], E2_data[3]);
    E2Write(E2_data, E2_CanCifg, 7);
    printE2Write(E2_data, E2_CanCifg, 7);

    //  设置屏蔽器0 1
    Set_Array_For_ID(E2_data, 0, CanCfg->RXM0ID, 0);
    Set_Array_For_ID(E2_data, 4, CanCfg->RXM1ID, 0);
    E2Write(E2_data, E2_RXM01ID, 8);
//    printE2Write(E2_data, E2_RXM01ID, 8);

    // 滤波器0、1， 首位为扩展帧标志位
    Set_Array_For_ID(E2_data, 0, CanCfg->RXF0ID, CanCfg->RXF0IDE);
    Set_Array_For_ID(E2_data, 4, CanCfg->RXF1ID, CanCfg->RXF1IDE);
    E2Write(E2_data, E2_RXF01, 8);
//    printE2Write(E2_data, E2_RXF01, 8);

    // 滤波器2、3
    Set_Array_For_ID(E2_data, 0, CanCfg->RXF2ID, CanCfg->RXF2IDE);
    Set_Array_For_ID(E2_data, 4, CanCfg->RXF3ID, CanCfg->RXF3IDE);
    E2Write(E2_data, E2_RXF23, 8);
//    printE2Write(E2_data, E2_RXF23, 8);

    // 滤波器4、5
    Set_Array_For_ID(E2_data, 0, CanCfg->RXF4ID, CanCfg->RXF4IDE);
    Set_Array_For_ID(E2_data, 4, CanCfg->RXF5ID, CanCfg->RXF5IDE);
    E2Write(E2_data, E2_RXF45, 8);
//    E2_data[0] = CanCfg->RXF4ID >> 24 | (CanCfg->RXF4IDE << 7);
//    E2_data[1] = (CanCfg->RXF4ID >> 16) & 0xFF;
//    E2_data[2] = (CanCfg->RXF4ID >> 8) & 0xFF;
//    E2_data[3] = CanCfg->RXF4ID & 0xFF;
//    E2_data[4] = (CanCfg->RXF5ID >> 24) | (CanCfg->RXF5IDE << 7);
//    E2_data[5] = (CanCfg->RXF5ID >> 16) & 0xFF;
//    E2_data[6] = (CanCfg->RXF5ID >> 8) & 0xFF;
//    E2_data[7] = CanCfg->RXF5ID & 0xFF;
//    E2Write(E2_data, E2_RXF45, 8);
//    printE2Write(E2_data, E2_RXF45, 8);
}

void Printf_E2(uint8 page) {
    uint8 i;
    uint8 E2_read_data[8];
    printf("page11: %02bX ", page);
    E2Read(E2_read_data, page, 8);
    for (i = 0; i < 8; i++) {
        printf(" %02bX ", E2_read_data[i]);
    }
    printf("\r\n");
}

void Printf_Cfg(CanCfgStruct *CanCfg) {
//    uint8 E2_read_data[8];
//    uint8 i;
    printf("page: ");
    Printf_E2(E2_CanCifg);
    Printf_E2(E2_RXF01);
    Printf_E2(E2_RXF23);
    Printf_E2(E2_RXF45);

//    printf("_5Kbps: %02bX \r\n", CanCfg->_5Kbps);
//    printf("bitrate: %02bX %02bX %02bX %02bX %02bX\r\n", CanCfg->bitrate[0],
//           CanCfg->bitrate[1], CanCfg->bitrate[2], CanCfg->bitrate[3], CanCfg->bitrate[4]);
//
//    printf("BUKT_enable: %02bX \r\n", CanCfg->BUKT_enable);
//    printf("CAN_MODE: %02bX \r\n", CanCfg->CAN_MODE);
//
//    printf("CANINTE: %02bX \r\n", CanCfg->CANINTE_enable);
//    printf("CANINTF: %02bX \r\n", CanCfg->CANINTF_enable);
//
//    printf("RXBnRXM0-1: %02bX %02bX\r\n", CanCfg->RXB0RXM, CanCfg->RXB1RXM);
//
//    printf("RXMnID0-1: %08lX %08lX\r\n", CanCfg->RXM0ID, CanCfg->RXM1ID);
//    printf("RXFnID0-5: %07lX %07lX %07lX %07lX %07lX\r\n", CanCfg->RXF0ID, CanCfg->RXF1ID, CanCfg->RXF2ID,
//           CanCfg->RXF3ID, CanCfg->RXF4ID, CanCfg->RXF5ID);
//
//    printf("RXFnIDE0-5: %bX %bX %bX %bX %bX\r\n", CanCfg->RXF0IDE, CanCfg->RXF1IDE, CanCfg->RXF2IDE, CanCfg->RXF3IDE,
//           CanCfg->RXF4IDE, CanCfg->RXF5IDE);
}

///*******************************************************************************
//* 描述    : 将数组中的数据，拼接完整ID，长度取4
//* 输入    : uint8 数组
//* 说明    : 无
//*******************************************************************************/
//uint32 GetInt32FormE2(uint8 *buf, uint8 addr) {
//    if (buf[1 + addr] & 0x8 >> 3) {
//        uint32 SID = ((uint32) buf[0 + addr] << 3) | (buf[1 + addr] >> 5);
//        uint32 EID = (uint32) (buf[1 + addr] & 3) << 16 | ((uint32) buf[2 + addr] << 8) | buf[3 + addr];
//        return SID << 18 | EID;
//    } else {
//        uint32 SID = (uint32) (buf[0 + addr] << 3) | (buf[1 + addr] >> 5);
//        return SID;
//    }
//}


/*******************************************************************************
* 描述    : 通过读取存储数据，设置Can配置
* 输入    : Can配置结构体
* 说明    : 依赖GetInt32FormE2拼接出完整ID， 首位做扩展标志位
*******************************************************************************/
void Set_Cfg_From_E2(CanCfgStruct *CanCfg) {
    uint8 E2_read_data[8];

    // page 0: Kbps, CAN_MODE, CANINTE, CANINTF, BUKT, RXB0RXM, RXB1RXM
    //  设置波特率
    E2Read(E2_read_data, E2_CanCifg, 8);  // 从 EEPROM 读取一段数据
    CanCfg->_5Kbps = E2_read_data[E2_5Kbps];
    Set_Bitrate_Array(CanCfg->_5Kbps, &(CanCfg->bitrate));
    CanCfg->CAN_MODE = E2_read_data[E2_CAN_MODE];      // 0:正常 1:休眠 2:环回 3:监听 4:配置
    CanCfg->CANINTE_enable = E2_read_data[E2_CANINTE_enable];
    CanCfg->CANINTF_enable = E2_read_data[E2_CANINTF_enable];
    //  设置滚存使能位、工作模式、中断使能位、中断标志位
    CanCfg->BUKT_enable = E2_read_data[E2_BUKT_enable];
    CanCfg->RXB0RXM = E2_read_data[E2_RXB0RXM];
    CanCfg->RXB1RXM = E2_read_data[E2_RXB1RXM];

    //  设置屏蔽器0 1
    E2Read(E2_read_data, E2_RXM01ID, 8);  // 从 EEPROM 读取一段数据
    CanCfg->RXM0ID = Get_ID_For_Array(E2_read_data, 0);
    CanCfg->RXM1ID = Get_ID_For_Array(E2_read_data, 4);

    //  滤波器0、1
    E2Read(E2_read_data, E2_RXF01, 8);  // 从 EEPROM 读取一段数据
    CanCfg->RXF0IDE = E2_read_data[1] & 0x8 >> 3;
    CanCfg->RXF0ID = Get_ID_For_Array(E2_read_data, 0);
    CanCfg->RXF1IDE = E2_read_data[4] & 0x8 >> 3;
    CanCfg->RXF1ID = Get_ID_For_Array(E2_read_data, 4);
    //  滤波器2、3
    E2Read(E2_read_data, E2_RXF23, 8);  // 从 EEPROM 读取一段数据
    CanCfg->RXF2IDE = E2_read_data[0] & 0x8 >> 3;
    CanCfg->RXF2ID = Get_ID_For_Array(E2_read_data, 0);
    CanCfg->RXF3IDE = E2_read_data[4] & 0x8 >> 3;
    CanCfg->RXF3ID = Get_ID_For_Array(E2_read_data, 4);
    //  滤波器4、5
    E2Read(E2_read_data, E2_RXF45, 8);  // 从 EEPROM 读取一段数据
    CanCfg->RXF4IDE = E2_read_data[0] & 0x8 >> 3;
    CanCfg->RXF4ID = Get_ID_For_Array(E2_read_data, 0);
    CanCfg->RXF5IDE = E2_read_data[4] & 0x8 >> 3;
    CanCfg->RXF5ID = Get_ID_For_Array(E2_read_data, 4);
}


/*******************************************************************************
* 描述    : 通过读取MCP2515寄存器，存储配置到E2
* 输入    : 无
* 说明    : 波特率配置数组默认采用树莓派上的组合，保留单位 5Kbps
*******************************************************************************/
void save_mcp2515_to_E2(void) {
    uint8 E2_data[8];
    uint8 i;
    uint8 offfset;

    uint8 tmp_data;
    uint8 EXIDE;
    uint32 ID;
    // 先读一次，保留波特率信息，波特率信息只能通过set-e2完成
    E2Read(E2_data, E2_CanCifg, 8);
//    E2_data[E2_5Kbps] =  E2Read(E2_data, E2_CanCifg, 8)[0];
    tmp_data = MCP2515_ReadByte(RXB0CTRL);
    E2_data[E2_BUKT_enable] = tmp_data & 0x7 >> 2;
    E2_data[E2_RXB0RXM] = tmp_data & RXM >> 5;
    E2_data[E2_RXB1RXM] = MCP2515_ReadByte(RXB1CTRL) & RXM >> 5;
    E2_data[E2_CAN_MODE] = MCP2515_ReadByte(CANCTRL) & REQOP >> 5;
    E2_data[E2_CANINTE_enable] = MCP2515_ReadByte(CANINTE);
    E2_data[E2_CANINTF_enable] = MCP2515_ReadByte(CANINTF);
    E2Write(E2_data, E2_CanCifg, 7);

    // 保存滤波器0-5和屏蔽器0-1的ID，及扩展帧标志位
    for (i = 0; i < 8; i++)
    {
        offfset = tmp_data * 4;
        EXIDE = MCP2515_ReadByte(RXF0SIDL + offfset) & 0x8 >> 3;
        ID = Get_ID_For_Buf(RXF0SIDH + offfset);
        Set_Array_For_ID(E2_data, offfset % 8, ID, EXIDE);
//        E2_data[0] = MCP2515_ReadByte(RXF0SIDH + tmp_data * 4);
//        E2_data[1] = MCP2515_ReadByte(RXF0SIDL + tmp_data * 4);
//        E2_data[2] = MCP2515_ReadByte(RXF0EID8 + tmp_data * 4);
//        E2_data[3] = MCP2515_ReadByte(RXF0EID0 + tmp_data * 4);
        E2Write(E2_data, E2_RXF01 + offfset, 4);
    }
}

// 测试时使用
//void SetCfg(CanCfgStruct *CanCfg)
//{
////    printf("SetCfg  : %02bX \r\n", main_status);
//    CanCfg->_5Kbps = 20;
////    printf("CanCfg->_5Kbps  : %02bX \r\n", CanCfg->_5Kbps);
////    {CAN_100Kbps,PRSEG_8TQ,PHSEG1_8TQ,PHSEG2_3TQ,SJW_1TQ}
//    CanCfg->bitrate[0] = CAN_100Kbps;
//    CanCfg->bitrate[1] = PRSEG_8TQ;
//    CanCfg->bitrate[2] = PHSEG1_8TQ;
//    CanCfg->bitrate[3] = PHSEG2_3TQ;
//    CanCfg->bitrate[4] = SJW_1TQ;
////    CanCfg->bitrate[0] = bitrate_100Kbps[0];
////    CanCfg->bitrate[1] = bitrate_100Kbps[1];
////    CanCfg->bitrate[2] = bitrate_100Kbps[2];
////    CanCfg->bitrate[3] = bitrate_100Kbps[3];
////    CanCfg->bitrate[4] = bitrate_100Kbps[4];
//
//    CanCfg->BUKT_enable = 1;
//    CanCfg->CAN_MODE = 2;       // 000 = 设定为正常工作模式
//                                // 001 = 设定为休眠模式
//                                // 010 = 设定为环回模式
//                                // 011 = 设定为仅监听模式
//                                // 100 = 设定为配置模式
//    CanCfg->CANINTE_enable = 3;
//    CanCfg->CANINTF_enable = 0;
//    CanCfg->RXM0ID = 0x1FFFFFFF;
//    CanCfg->RXM1ID = 0x1FFFFFFF;
//    printf("CanCfg->0RXM1ID  : %08bX \r\n", CanCfg->RXM1ID);
//    CanCfg->RXF0ID = 0x100;
//    printf("CanCfg->RXF0ID  : %08bX \r\n", CanCfg->RXF0ID);
//    CanCfg->RXF1ID = 0x7FE;
//    CanCfg->RXF2ID = 0x101;
//    CanCfg->RXF3ID = 0x102;
//    CanCfg->RXF4ID = 0x103;
//    CanCfg->RXF5ID = 0x104;
//
//    CanCfg->RXF0IDE = 1;
//    CanCfg->RXF1IDE = 0;
//    CanCfg->RXF2IDE = 0;
//    CanCfg->RXF3IDE = 1;
//    CanCfg->RXF4IDE = 0;
//    CanCfg->RXF5IDE = 1;
////    printf("CanCfg->RXF5ID  : %02bX \r\n", CanCfg->RXF5ID);
//}

//void ReadCfg(void) {
//    printf("CNF1: %02bX ", MCP2515_ReadByte(CNF1));
//    printf("CNF2: %02bX ", MCP2515_ReadByte(CNF2));
//    printf("CNF3: %02bX \r\n", MCP2515_ReadByte(CNF3));
//    printf("RXB0CTRL: %02bX ", MCP2515_ReadByte(RXB0CTRL));
//    printf("CANINTF: %02bX ", MCP2515_ReadByte(CANINTF));
//    printf("CANINTE: %02bX \r\n", MCP2515_ReadByte(CANINTE));
//    printf("RXF0SIDH: %02bX ", MCP2515_ReadByte(RXF0SIDH));
//    printf("RXF1SIDH: %02bX ", MCP2515_ReadByte(RXF1SIDH));
//    printf("RXF2SIDH: %02bX \r\n", MCP2515_ReadByte(RXF2SIDH));
//    printf("RXF3SIDH: %02bX ", MCP2515_ReadByte(RXF3SIDH));
//    printf("RXF4SIDH: %02bX ", MCP2515_ReadByte(RXF4SIDH));
//    printf("RXF5SIDH: %02bX \r\n", MCP2515_ReadByte(RXF5SIDH));
//    printf("RXM0SIDH: %02bX ", MCP2515_ReadByte(RXM0SIDH));
//    printf("RXM1SIDH: %02bX ", MCP2515_ReadByte(RXM1SIDH));
//    printf("CANCTRL: %02bX \r\n", MCP2515_ReadByte(CANCTRL));
//}

/*******************************************************************************
* 函数名  : 上电初始化程序
* 描述    : 上电配置
* 说明    : 设计上电自检程序，
*******************************************************************************/
void power_on_init(CanCfgStruct *CanCfg) {
    //    MCP2515_Init(bitrate_100Kbps);
//    printf("power_on_init: %02bX \r\n", main_status);
//    SetCfg(&CanCfg);
//    Printf_Msg(&RecMsg);
//    printf("SetCfg: %02bX \r\n", main_status);
//    SaveCfgToE2(&CanCfg);
    // page 0: Kbps, CAN_MODE, CANINTE, CANINTF, BUKT, RXB0RXM, RXB1RXM
//    uint8 Send_data[8] = {0x14, 0x5, 0x03, 0x0, 0x01, 0x05, 0x03, 0x0};
//    uint8 Send_data1[8] = {0x1F, 0xFF, 0xFF, 0xFF, 0x1F, 0xFF, 0xFF, 0xFF};
//    uint8 Send_data2[8] = {0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01};
//    uint8 Send_data3[8] = {0x00, 0x00, 0x07, 0xff, 0x10, 0x00, 0x17, 0xFF};
//    uint8 Send_data4[8] = {0x9F, 0xFF, 0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0xFE};
//    E2Write(Send_data, E2_CanCifg, 7);
//    E2Write(Send_data1, E2_RXM01ID, 8);
//    // 1 100  0 101
//    E2Write(Send_data2, E2_RXF01, 8);
//    // 0 7FF  1 17FF
//    E2Write(Send_data3, E2_RXF23, 8);
//    // 1 1FFFFfFF  1 1FFFFfFE
//    E2Write(Send_data4, E2_RXF45, 8);


    printf("SaveCfgToE2: %02bX \r\n", main_status);
//    Set_Cfg_From_E2(&CanCfg);
    Printf_Cfg(&CanCfg);
    printf("Printf_Cfg: %02bX \r\n", main_status);
//    Can_Init(&CanCfg);
}

/*******************************************************************************
* 描述    : 将can配置发送给系统
* 说明    :　设计发送描述信息
*******************************************************************************/
void send_cfg(uint32 ID, uint8 flag, uint8 EXIDE, uint8 num, MsgStruct *SendMsg) {
    SendMsg->ID = ID;
    SendMsg->EXIDE = EXIDE;
    SendMsg->DATA[1] = flag;
    SendMsg->DATA[2] = num;
    Send(&SendMsg);
}

/*******************************************************************************
* 描述    : 将can配置发送给系统
* 说明    : 设计发送描述信息
*******************************************************************************/
void send_can_cfg(CanCfgStruct *CanCfg, MsgStruct *SendMsg) {
    //　上电反馈，设计位远程帧，长度为7
    SendMsg->RTR = 0x1;
    SendMsg->DLC = 0;  // 初始设置位0，广播上电
    // 用第0个滤波器发送零长msg,通知上电
    SendMsg->ID = CanCfg->RXF0ID;
    SendMsg->EXIDE = CanCfg->RXF0IDE;
    Send(&SendMsg);

    SendMsg->DLC = 7;
    SendMsg->DATA[0] = action_send_can_cfg;
    Set_Array_For_ID(&(SendMsg->DATA), 3, CanCfg->RXF0ID, CanCfg->RXF0IDE);

//    SendMsg->DATA[3] = CanCfg->RXF0ID >> 24;
//    SendMsg->DATA[4] = (CanCfg->RXF0ID >> 16) & 0xFF;
//    SendMsg->DATA[5] = (CanCfg->RXF0ID >> 8) & 0xFF;
//    SendMsg->DATA[6] = CanCfg->RXF0ID & 0xFF;

    send_cfg(CanCfg->RXF0ID, E2_5Kbps, CanCfg->RXF0IDE, CanCfg->_5Kbps, &SendMsg);
    send_cfg(CanCfg->RXF0ID, E2_BUKT_enable, CanCfg->RXF0IDE, CanCfg->BUKT_enable, &SendMsg);
    send_cfg(CanCfg->RXF0ID, E2_CAN_MODE, CanCfg->RXF0IDE, CanCfg->CAN_MODE, &SendMsg);
    send_cfg(CanCfg->RXF0ID, E2_CANINTE_enable, CanCfg->RXF0IDE, CanCfg->CANINTE_enable, &SendMsg);
    send_cfg(CanCfg->RXF0ID, E2_CANINTF_enable, CanCfg->RXF0IDE, CanCfg->CANINTF_enable, &SendMsg);

    send_cfg(CanCfg->RXF1ID, E2_RXF0ID, CanCfg->RXF0IDE, 0, &SendMsg);
    send_cfg(CanCfg->RXF2ID, E2_RXF1ID, CanCfg->RXF1IDE, 0, &SendMsg);
    send_cfg(CanCfg->RXF3ID, E2_RXF2ID, CanCfg->RXF2IDE, 0, &SendMsg);
    send_cfg(CanCfg->RXF4ID, E2_RXF3ID, CanCfg->RXF3IDE, 0, &SendMsg);
    send_cfg(CanCfg->RXF5ID, E2_RXF4ID, CanCfg->RXF4IDE, 0, &SendMsg);
    send_cfg(CanCfg->RXF5ID, E2_RXF5ID, CanCfg->RXF5IDE, 0, &SendMsg);
    send_cfg(CanCfg->RXM0ID, E2_RXM0ID, 0, 0, &SendMsg);
    send_cfg(CanCfg->RXM1ID, E2_RXM1ID, 0, 0, &SendMsg);
}

/*******************************************************************************
* 函数名  : main
* 描述    : 主函数，用户程序从main函数开始运行
* 输入    : 无
* 输出    : 无
* 返回值  : 无
* 说明    : 设计串口调试模块
*******************************************************************************/
void main(void) {
    uint32 ID = 0x101;
    uint8 EXIDE = 0;
    uint8 DLC = 8;
    uint8 i;

//    uint8 CANINTF_Flag;
//    uint8 Send_data[] = {0x20, 0xF1, 0x03, 0x03, 0x04, 0x05, 0x06, 0x07};
//    uint8 E2_data[8];

//    MsgStruct SendMsg;
//    MsgStruct RecMsg;

    CanCfgStruct CanCfg;

    //    初始设置配置
    UART_init();    //UART1初始化配置
    Exint_Init();   //外部中断1初始化函数
    power_on_init(&CanCfg);
    while (1) {
        Delay_Nms(2000);
//        switch (main_status) {
//            case main_power_on:  // 上电自检
//                printf("page1: %02bX %02bX ", main_status, main_power_on);
//                power_on_init(&CanCfg);
////            case main_set_can_cfg: // 设置con config
////                Set_Cfg_From_E2(&CanCfg);
////                Can_Init(&CanCfg);
////            case main_send_can_cfg: // CAN发送当前配置，并打印
////                send_can_cfg(&CanCfg, &SendMsg);
////                Printf_Cfg(&CanCfg);     // 通过读取ＭＣＰ２５１５打印全部的配置信息
////                main_status = 0;  // 进入default模式
////                break;
////            case main_save_cfg:  // 将Config保持到E2中
////                save_mcp2515_to_E2();
////                break;
//            default:  // 默认进入轮询等待
//                // 扫描GPIO，进入发送程序 scan_GPIO_chanage()
//                // 扫描接收器状态，进入接收程序 scan_rec_chanage()
//                break;
//        }
    }


//
//    E2Write(Send_data, 0xF0, sizeof(Send_data));
//    printf("Main Set");
//    printE2Write(Send_data, 0xF0, sizeof(Send_data));
//    E2Read(E2_data, E2_RXM01ID, sizeof(E2_data));  // 从 EEPROM 读取一段数据
//    printf("Main Read ");
//    printE2Write(E2_data, E2_RXM01ID, sizeof(E2_data));
//    for (i = 0; i < 8; i++) //发送字符串，直到遇到0才结束
//        {
//        printf("读取数据 : E2_data[%bd] = %bx \r\n", i, E2_data[i]);
//        }
//    ReadCfg();
//
//    SendMsg.IsSend = 0x1;
//
//    SendMsg.ID = ID;
//    SendMsg.TYPE = 0x2;
//    SendMsg.EXIDE = 0x0;
//    SendMsg.DLC = 8;
//
//    for (i = 0; i < 8; i++) //发送字符串，直到遇到0才结束
//    {
//        SendMsg.DATA[i] = Send_data[i];
////        printf("SendMsg.DATA[%bd] = %bx \r\n", i, SendMsg.DATA[i]);
//    }
//
//    RecMsg.IsSend = 0;
////    Send(&SendMsg);
//
//    for (i = 0; i < 2; i++) //发送字符串，直到遇到0才结束
//    {
//        Send(&SendMsg);
//        Printf_Msg(&SendMsg);
//        SendMsg.ID = 0x100;
//        SendMsg.TYPE = 0x2;
//        SendMsg.EXIDE = 0x1;
//        SendMsg.DLC = 7;
//
//        Delay_Nms(3000);
//
//        printf("CAN_RX0IF_Flag = %bd \r\n", CAN_RX0IF_Flag);
//        printf("CAN_RX1IF_Flag = %bd \r\n", CAN_RX1IF_Flag);
//        printf("CANSTAT: %02bX \r\n", MCP2515_ReadByte(CANSTAT));
//
//
//        CANINTF_Flag = MCP2515_ReadByte(CANINTF);
//        printf("CANINTF: %02bX \r\n", CANINTF_Flag);
//
//        if (CANINTF_Flag & RX0IF) {
//            Receive(RXB0CTRL, &RecMsg);
//            Printf_Msg(&RecMsg);
//            MCP2515_WriteByte(CANINTF, MCP2515_ReadByte(CANINTF) & 0xFE);//清除中断标志位(中断标志寄存器必须由MCU清零)
//        }
//
//        if (CANINTF_Flag & RX1IF) {
//            Receive(RXB1CTRL, &RecMsg);
//            Printf_Msg(&RecMsg);
//            MCP2515_WriteByte(CANINTF, MCP2515_ReadByte(CANINTF) & 0xFD);//清除中断标志位(中断标志寄存器必须由MCU清零)
//        }
//    }
//
//    Delay_Nms(2000);
//
//    while (1) {
//
////        if (CAN_RX0IF_Flag == 1)                            //接收缓冲器0 满中断标志位
////        {
////            CAN_RX0IF_Flag = 0;//CAN接收到数据标志
////            Receive(RXB0CTRL, RXB_Value);//CAN接收一帧数据
////            Delay_Nms(2000);  //移动到下一个字符
////
////        }
////        if (CAN_RX1IF_Flag == 1)                            //接收缓冲器1 满中断标志位
////        {
////            CAN_RX1IF_Flag = 0;//CAN接收到数据标志
////            CAN_Receive_Buffer(RXB1CTRL, RXB_Value);//CAN接收一帧数据
//////            UART_send_buffer(RXB_Value, 14); //发送一个字符
////            Delay_Nms(2000);  //移动到下一个字符
//////			UART_send_buffer(RXB_Value,14); //发送一个字符
////        }
////
////        Delay_Nms(2000);
//    }

}