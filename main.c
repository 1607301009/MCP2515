#include <reg52.h>
#include "MCP2515.H"
#include <stdio.h>


//��������
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

// IDת��ģ��
extern uint32 Get_ID_For_Array(uint8 *array, uint8 offset);
extern void Set_Array_For_ID(uint8 *array, uint8 offset, uint32 ID, uint8 EXIDE);
extern uint32 Get_ID_For_Buf(uint8 buf_addr);
extern void Set_Buf_For_ID(uint8 buf_addr, uint32 ID, uint8 EXIDE);

extern void Set_Bitrate_Array(uint8 _5Kbps, uint8 *bitrate);

// �洢ģ��
extern void E2Read(unsigned char *buf, unsigned char addr, unsigned char len);
extern void E2Write(unsigned char *buf, unsigned char addr, unsigned char len);

//bool CAN_MERRF_Flag = 0;                            //CAN���Ĵ����жϱ�־λ
//bool CAN_WAKIF_Flag = 0;                            //CAN�����жϱ�־λ
//bool CAN_ERRIF_Flag = 0;                            //CAN�����жϱ�־λ��EFLG �Ĵ������ж���ж�Դ��
//bool CAN_TX2IF_Flag = 0;                            //MCP2515���ͻ�����2 ���жϱ�־λ
//bool CAN_TX1IF_Flag = 0;                            //MCP2515���ͻ�����1 ���жϱ�־λ
//bool CAN_TX0IF_Flag = 0;                            //MCP2515���ͻ�����0 ���жϱ�־λ
bool CAN_RX1IF_Flag = false;                        //MCP2515���ջ�����1 ���жϱ�־λ
bool CAN_RX0IF_Flag = false;                        //MCP2515���ջ�����0 ���жϱ�־λ

uint8 main_status = 1;

/*******************************************************************************
* ����    : �ض���putchar, ��printf������ӡ�������С�
*******************************************************************************/
char putchar(char c)
{
    UART_send_str(c);
    return c;
}

/*******************************************************************************
* ������  : Exint_Init
* ����    : �ⲿ�ж�1��ʼ������
*******************************************************************************/
void Exint_Init(void) {
    PX1 = 1;    //�����ⲿ�ж�1���ж����ȼ�Ϊ�����ȼ�
    IT1 = 1;    //����INT1���ж����� (1:���½��� 0:�����غ��½���)
    EX1 = 1;    //ʹ��INT1�ж�
    EA = 1;     //ʹ�����ж�
}

/*******************************************************************************
* ������  : Exint_ISR
* ����    : �ⲿ�ж�1�жϷ����� ��Ƭ������P3.3��MCP2515 INT����
* ˵��    : ���ڼ��MCP2515�ж����ŵ��ж��ź�
*******************************************************************************/
void Exint_ISR(void) interrupt 2 using 1
{
    uint8 Flag;                                //CAN���յ����ݱ�־
    Flag = MCP2515_ReadByte(CANINTF);

//    if (Flag&0x80) CAN_MERRF_Flag = 1;                            //CAN���Ĵ����жϱ�־λ
//    if (Flag&0x40) CAN_WAKIF_Flag = 1;                            //CAN�����жϱ�־λ
//    if (Flag&0x20) CAN_ERRIF_Flag = 1;                            //CAN�����жϱ�־λ��EFLG �Ĵ������ж���ж�Դ��
//    if (Flag&0x10) CAN_TX2IF_Flag = 1;                            //MCP2515���ͻ�����2 ���жϱ�־λ
//    if (Flag&0x08) CAN_TX1IF_Flag = 1;                            //MCP2515���ͻ�����1 ���жϱ�־λ
//    if (Flag&0x04) CAN_TX0IF_Flag = 1;                            //MCP2515���ͻ�����0 ���жϱ�־λ
    if (Flag&0x02) CAN_RX1IF_Flag = true;                            //MCP2515���ջ�����1 ���жϱ�־λ
    if (Flag&0x01) CAN_RX0IF_Flag = true;                           //MCP2515���ջ�����0 ���жϱ�־λ
}

/*******************************************************************************
* ����    : ��msg��ӡ����
* ����    : Msg�ṹ��
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

/* ���õ��Ա�־λ������Ҫ���͵����� ת����uart */
void Send(MsgStruct *SendMsg) {
    uint32 ID = SendMsg->ID;
    uint8 EXIDE = SendMsg->EXIDE;
    uint8 DLC = SendMsg->DLC | (SendMsg->RTR << 6);
    CAN_Send_buffer(ID, EXIDE, DLC, SendMsg->DATA);
}


/*******************************************************************************
* ����    : ͨ��Can msg����CAN����,
* data[0]������־λ��
* data[1]:addr, 2: data_len, [3:7]:data_arr
* ˵��    : ������ɺ����·���ʼ��ʹ���źš�
* ����ģ�������
*******************************************************************************/
void msg_set_E2(uint8 *msg_data) {
    uint8 i;
    uint8 addr = msg_data[1];
    uint8 len = msg_data[2];
    uint8 array[8];
    for (i = 0; i < len; i++) //�����ַ�����ֱ������0�Ž���
        {
        array[i] = msg_data[i+3];
        }
    E2Write(array, addr, len + 3);
}

/*******************************************************************************
* ����    : ���յ���Ϣ��,����״̬���з�Ӧ
* ����    : msg
* ˵��    : ���äӦ��ģʽ�� ͳһһ��ID���ϱ��͵��ԣ���������豸
*******************************************************************************/
//void action_rec_msg(MsgStruct *RecMsg) {
//    if (RecMsg->FILHIT == 0) {  // �˲���0H ���� ������Ϣ
//        if (RecMsg->DATA[0] == action_status) {
//            // ��������������״̬�� ʹ�����ã� ��ȡ���õ�
//            main_status =RecMsg->DATA[1];
//        } else if (RecMsg->DATA[0] == action_E2) {
//            // ��rec������E2�� config, ������ֻ��ͨ��E2д �ķ�ʽ�޸�
//            msg_set_E2(RecMsg->DATA);
//        } else if (RecMsg->DATA[0] == action_MCP2515){  // ֱ������MCP2515�Ĵ���
//            // [0] ״̬��־λ�� [1] addr, [2] data
//            MCP2515_WriteByte(RecMsg->DATA[1], RecMsg->DATA[2]);
//        }
//    } else if (RecMsg->FILHIT == 1) {  // �˲���1H ��ȡMCP2515����
//        if (RecMsg->DATA[0] == action_status) {
//            // ��ȡ����������״̬
//            RecMsg->DLC = 1;
//            RecMsg->DATA[0] == main_status;
//        } else if (RecMsg->DATA[0] == action_E2) {
//            // ��rec������E2�� config
//            RecMsg->DLC = RecMsg->DATA[2];
//            E2Read(RecMsg->DATA, RecMsg->DATA[1], RecMsg->DATA[2]);  // �� EEPROM ��ȡһ������
//            // ����Ӧ��msg
//        } else if (RecMsg->DATA[0] == action_MCP2515) {  // ֱ�Ӷ�ȡMCP2515�Ĵ���
//            RecMsg->DLC = 1;
//            RecMsg->DATA[0] == MCP2515_ReadByte(RecMsg->DATA[1]);
//        }
//        if (RecMsg->RTR  == 0) {
//            // �����Զ��֡��˵����Ӧ�����
//            RecMsg->RTR = 1; // Ӧ��ģʽ���ΪԶ��֡�� ����ػ�ģʽ�ظ���������
//            Send(&RecMsg);
//        }
//        Printf_Msg(&RecMsg);
//    } else if (RecMsg->FILHIT == 5) {  // �˲���5H äӦ��ģʽ
//        return;
//    } else {  // GPIO ����
//        return;
//    }
//}
//
///* ����Ҫ���͵����� ת����uart, CAN_RX_Buf[14]*/
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
//    for (i = 0; i < RecMsg->DLC; i++) //��ȡ���յ�������
//    {
//        RecMsg->DATA[i] = MCP2515_ReadByte(RXB_CTRL_Address + 6 + i);
//    }
//    // ���ݽ��յ�msg, ���ж���
//    action_rec_msg(&RecMsg);
//}

void printE2Write(uint8 *E2_data, uint8 add, uint8 Len) {
    uint8 i;
    printf("add: %02bX Len: %02bX Data:", add, Len);
    for (i = 0; i < Len; i++) //�����ַ�����ֱ������0�Ž���
        {
        printf(" %02bX", E2_data[i]);
        }
    printf("\r\n");
}

// ���ص���ʹ��
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

    //  ����������0 1
    Set_Array_For_ID(E2_data, 0, CanCfg->RXM0ID, 0);
    Set_Array_For_ID(E2_data, 4, CanCfg->RXM1ID, 0);
    E2Write(E2_data, E2_RXM01ID, 8);
//    printE2Write(E2_data, E2_RXM01ID, 8);

    // �˲���0��1�� ��λΪ��չ֡��־λ
    Set_Array_For_ID(E2_data, 0, CanCfg->RXF0ID, CanCfg->RXF0IDE);
    Set_Array_For_ID(E2_data, 4, CanCfg->RXF1ID, CanCfg->RXF1IDE);
    E2Write(E2_data, E2_RXF01, 8);
//    printE2Write(E2_data, E2_RXF01, 8);

    // �˲���2��3
    Set_Array_For_ID(E2_data, 0, CanCfg->RXF2ID, CanCfg->RXF2IDE);
    Set_Array_For_ID(E2_data, 4, CanCfg->RXF3ID, CanCfg->RXF3IDE);
    E2Write(E2_data, E2_RXF23, 8);
//    printE2Write(E2_data, E2_RXF23, 8);

    // �˲���4��5
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
//* ����    : �������е����ݣ�ƴ������ID������ȡ4
//* ����    : uint8 ����
//* ˵��    : ��
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
* ����    : ͨ����ȡ�洢���ݣ�����Can����
* ����    : Can���ýṹ��
* ˵��    : ����GetInt32FormE2ƴ�ӳ�����ID�� ��λ����չ��־λ
*******************************************************************************/
void Set_Cfg_From_E2(CanCfgStruct *CanCfg) {
    uint8 E2_read_data[8];

    // page 0: Kbps, CAN_MODE, CANINTE, CANINTF, BUKT, RXB0RXM, RXB1RXM
    //  ���ò�����
    E2Read(E2_read_data, E2_CanCifg, 8);  // �� EEPROM ��ȡһ������
    CanCfg->_5Kbps = E2_read_data[E2_5Kbps];
    Set_Bitrate_Array(CanCfg->_5Kbps, &(CanCfg->bitrate));
    CanCfg->CAN_MODE = E2_read_data[E2_CAN_MODE];      // 0:���� 1:���� 2:���� 3:���� 4:����
    CanCfg->CANINTE_enable = E2_read_data[E2_CANINTE_enable];
    CanCfg->CANINTF_enable = E2_read_data[E2_CANINTF_enable];
    //  ���ù���ʹ��λ������ģʽ���ж�ʹ��λ���жϱ�־λ
    CanCfg->BUKT_enable = E2_read_data[E2_BUKT_enable];
    CanCfg->RXB0RXM = E2_read_data[E2_RXB0RXM];
    CanCfg->RXB1RXM = E2_read_data[E2_RXB1RXM];

    //  ����������0 1
    E2Read(E2_read_data, E2_RXM01ID, 8);  // �� EEPROM ��ȡһ������
    CanCfg->RXM0ID = Get_ID_For_Array(E2_read_data, 0);
    CanCfg->RXM1ID = Get_ID_For_Array(E2_read_data, 4);

    //  �˲���0��1
    E2Read(E2_read_data, E2_RXF01, 8);  // �� EEPROM ��ȡһ������
    CanCfg->RXF0IDE = E2_read_data[1] & 0x8 >> 3;
    CanCfg->RXF0ID = Get_ID_For_Array(E2_read_data, 0);
    CanCfg->RXF1IDE = E2_read_data[4] & 0x8 >> 3;
    CanCfg->RXF1ID = Get_ID_For_Array(E2_read_data, 4);
    //  �˲���2��3
    E2Read(E2_read_data, E2_RXF23, 8);  // �� EEPROM ��ȡһ������
    CanCfg->RXF2IDE = E2_read_data[0] & 0x8 >> 3;
    CanCfg->RXF2ID = Get_ID_For_Array(E2_read_data, 0);
    CanCfg->RXF3IDE = E2_read_data[4] & 0x8 >> 3;
    CanCfg->RXF3ID = Get_ID_For_Array(E2_read_data, 4);
    //  �˲���4��5
    E2Read(E2_read_data, E2_RXF45, 8);  // �� EEPROM ��ȡһ������
    CanCfg->RXF4IDE = E2_read_data[0] & 0x8 >> 3;
    CanCfg->RXF4ID = Get_ID_For_Array(E2_read_data, 0);
    CanCfg->RXF5IDE = E2_read_data[4] & 0x8 >> 3;
    CanCfg->RXF5ID = Get_ID_For_Array(E2_read_data, 4);
}


/*******************************************************************************
* ����    : ͨ����ȡMCP2515�Ĵ������洢���õ�E2
* ����    : ��
* ˵��    : ��������������Ĭ�ϲ�����ݮ���ϵ���ϣ�������λ 5Kbps
*******************************************************************************/
void save_mcp2515_to_E2(void) {
    uint8 E2_data[8];
    uint8 i;
    uint8 offfset;

    uint8 tmp_data;
    uint8 EXIDE;
    uint32 ID;
    // �ȶ�һ�Σ�������������Ϣ����������Ϣֻ��ͨ��set-e2���
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

    // �����˲���0-5��������0-1��ID������չ֡��־λ
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

// ����ʱʹ��
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
//    CanCfg->CAN_MODE = 2;       // 000 = �趨Ϊ��������ģʽ
//                                // 001 = �趨Ϊ����ģʽ
//                                // 010 = �趨Ϊ����ģʽ
//                                // 011 = �趨Ϊ������ģʽ
//                                // 100 = �趨Ϊ����ģʽ
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
* ������  : �ϵ��ʼ������
* ����    : �ϵ�����
* ˵��    : ����ϵ��Լ����
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
* ����    : ��can���÷��͸�ϵͳ
* ˵��    :����Ʒ���������Ϣ
*******************************************************************************/
void send_cfg(uint32 ID, uint8 flag, uint8 EXIDE, uint8 num, MsgStruct *SendMsg) {
    SendMsg->ID = ID;
    SendMsg->EXIDE = EXIDE;
    SendMsg->DATA[1] = flag;
    SendMsg->DATA[2] = num;
    Send(&SendMsg);
}

/*******************************************************************************
* ����    : ��can���÷��͸�ϵͳ
* ˵��    : ��Ʒ���������Ϣ
*******************************************************************************/
void send_can_cfg(CanCfgStruct *CanCfg, MsgStruct *SendMsg) {
    //���ϵ練�������λԶ��֡������Ϊ7
    SendMsg->RTR = 0x1;
    SendMsg->DLC = 0;  // ��ʼ����λ0���㲥�ϵ�
    // �õ�0���˲��������㳤msg,֪ͨ�ϵ�
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
* ������  : main
* ����    : ���������û������main������ʼ����
* ����    : ��
* ���    : ��
* ����ֵ  : ��
* ˵��    : ��ƴ��ڵ���ģ��
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

    //    ��ʼ��������
    UART_init();    //UART1��ʼ������
    Exint_Init();   //�ⲿ�ж�1��ʼ������
    power_on_init(&CanCfg);
    while (1) {
        Delay_Nms(2000);
//        switch (main_status) {
//            case main_power_on:  // �ϵ��Լ�
//                printf("page1: %02bX %02bX ", main_status, main_power_on);
//                power_on_init(&CanCfg);
////            case main_set_can_cfg: // ����con config
////                Set_Cfg_From_E2(&CanCfg);
////                Can_Init(&CanCfg);
////            case main_send_can_cfg: // CAN���͵�ǰ���ã�����ӡ
////                send_can_cfg(&CanCfg, &SendMsg);
////                Printf_Cfg(&CanCfg);     // ͨ����ȡ�ͣãУ���������ӡȫ����������Ϣ
////                main_status = 0;  // ����defaultģʽ
////                break;
////            case main_save_cfg:  // ��Config���ֵ�E2��
////                save_mcp2515_to_E2();
////                break;
//            default:  // Ĭ�Ͻ�����ѯ�ȴ�
//                // ɨ��GPIO�����뷢�ͳ��� scan_GPIO_chanage()
//                // ɨ�������״̬��������ճ��� scan_rec_chanage()
//                break;
//        }
    }


//
//    E2Write(Send_data, 0xF0, sizeof(Send_data));
//    printf("Main Set");
//    printE2Write(Send_data, 0xF0, sizeof(Send_data));
//    E2Read(E2_data, E2_RXM01ID, sizeof(E2_data));  // �� EEPROM ��ȡһ������
//    printf("Main Read ");
//    printE2Write(E2_data, E2_RXM01ID, sizeof(E2_data));
//    for (i = 0; i < 8; i++) //�����ַ�����ֱ������0�Ž���
//        {
//        printf("��ȡ���� : E2_data[%bd] = %bx \r\n", i, E2_data[i]);
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
//    for (i = 0; i < 8; i++) //�����ַ�����ֱ������0�Ž���
//    {
//        SendMsg.DATA[i] = Send_data[i];
////        printf("SendMsg.DATA[%bd] = %bx \r\n", i, SendMsg.DATA[i]);
//    }
//
//    RecMsg.IsSend = 0;
////    Send(&SendMsg);
//
//    for (i = 0; i < 2; i++) //�����ַ�����ֱ������0�Ž���
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
//            MCP2515_WriteByte(CANINTF, MCP2515_ReadByte(CANINTF) & 0xFE);//����жϱ�־λ(�жϱ�־�Ĵ���������MCU����)
//        }
//
//        if (CANINTF_Flag & RX1IF) {
//            Receive(RXB1CTRL, &RecMsg);
//            Printf_Msg(&RecMsg);
//            MCP2515_WriteByte(CANINTF, MCP2515_ReadByte(CANINTF) & 0xFD);//����жϱ�־λ(�жϱ�־�Ĵ���������MCU����)
//        }
//    }
//
//    Delay_Nms(2000);
//
//    while (1) {
//
////        if (CAN_RX0IF_Flag == 1)                            //���ջ�����0 ���жϱ�־λ
////        {
////            CAN_RX0IF_Flag = 0;//CAN���յ����ݱ�־
////            Receive(RXB0CTRL, RXB_Value);//CAN����һ֡����
////            Delay_Nms(2000);  //�ƶ�����һ���ַ�
////
////        }
////        if (CAN_RX1IF_Flag == 1)                            //���ջ�����1 ���жϱ�־λ
////        {
////            CAN_RX1IF_Flag = 0;//CAN���յ����ݱ�־
////            CAN_Receive_Buffer(RXB1CTRL, RXB_Value);//CAN����һ֡����
//////            UART_send_buffer(RXB_Value, 14); //����һ���ַ�
////            Delay_Nms(2000);  //�ƶ�����һ���ַ�
//////			UART_send_buffer(RXB_Value,14); //����һ���ַ�
////        }
////
////        Delay_Nms(2000);
//    }

}