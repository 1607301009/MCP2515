#include <reg52.h>
#include "MCP2515.H"
#include <stdio.h>


//��������
extern void UART_init(void);
extern void UART_send_str(uint8 d);
extern void UART_send_buffer(uint8 *buffer,uint16 len);

extern void Delay_Nms(uint16 x);

extern uint8 MCP2515_ReadByte(uint8 addr);
extern void MCP2515_WriteByte(uint8 addr,uint8 dat);
extern void MCP2515_Init(uint8 *CAN_Bitrate);
extern void Can_Init(CanCfgStruct *CanCfg);
extern void CAN_Send_buffer(uint32 ID,uint8 EXIDE,uint8 DLC,uint8 *Send_data);
extern void CAN_Receive_Buffer(uint8 RXB_CTRL_Address,uint8 *CAN_RX_Buf);


bool CAN_MERRF_Flag = 0;                            //CAN���Ĵ����жϱ�־λ
bool CAN_WAKIF_Flag = 0;                            //CAN�����жϱ�־λ
bool CAN_ERRIF_Flag = 0;                            //CAN�����жϱ�־λ��EFLG �Ĵ������ж���ж�Դ��
bool CAN_TX2IF_Flag = 0;                            //MCP2515���ͻ�����2 ���жϱ�־λ
bool CAN_TX1IF_Flag = 0;                            //MCP2515���ͻ�����1 ���жϱ�־λ
bool CAN_TX0IF_Flag = 0;                            //MCP2515���ͻ�����0 ���жϱ�־λ
bool CAN_RX1IF_Flag = false;                            //MCP2515���ջ�����1 ���жϱ�־λ
bool CAN_RX0IF_Flag = false;                            //MCP2515���ջ�����0 ���жϱ�־λ



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
    if (Flag&0x02) CAN_RX1IF_Flag = true;                            //MCP2515���ջ�����1 ���жϱ�־λ
    if (Flag&0x01) CAN_RX0IF_Flag = true;                           //MCP2515���ջ�����0 ���жϱ�־λ
}

void ShowMsg(MsgStruct *Msg) {
    uint8 i;
    uint32 ID = Msg->ID;
    uint8 EXIDE = Msg->EXIDE;
    uint8 DLC = Msg->DLC;

    if (Msg->IsSend) {
        printf("Can send    ");
    } else {
        printf("Can recevie ");
    }

    if (EXIDE) {
        printf("ID: %07lX,  DLC:%bx,  Data: ", ID, DLC);
    } else {
        printf("ID: %7lX,  DLC:%bx,  Data: ", ID, DLC);
    }

    for (i = 0; i < DLC; i++) {
        printf("%02bX ", Msg->DATA[i]);
    }

    printf("\r\n");
}

/* ����Ҫ���͵����� ת����uart */
void Send(MsgStruct *SendMsg) {
    uint32 ID = SendMsg->ID;
    uint8 EXIDE = SendMsg->EXIDE;
    uint8 DLC = SendMsg->DLC;
//    ShowMsg(SendMsg);
    CAN_Send_buffer(ID, EXIDE, DLC, SendMsg->DATA);
}

/* ����Ҫ���͵����� ת����uart, CAN_RX_Buf[14]*/
void Receive(uint8 RXB_CTRL_Address, MsgStruct *RecMsg) {
    uint8 i;

    uint8 RXBnCTRL = MCP2515_ReadByte(RXB_CTRL_Address);
    uint8 RXBnSIDH = MCP2515_ReadByte(RXB_CTRL_Address + 1);
    uint8 RXBnSIDL = MCP2515_ReadByte(RXB_CTRL_Address + 2);
    uint8 RXBnEID8 = MCP2515_ReadByte(RXB_CTRL_Address + 3);
    uint8 RXBnEID0 = MCP2515_ReadByte(RXB_CTRL_Address + 4);
    uint8 RXBnDLC = MCP2515_ReadByte(RXB_CTRL_Address + 5);

    RecMsg->EXIDE = (RXBnSIDL & 0x8) >> 3;  // ��չ��ʶ����־λ 1 = �յ��ı�������չ֡, 0 = �յ��ı����Ǳ�׼֡
    RecMsg->DLC = RXBnDLC & 0x0F;

    if (RecMsg->EXIDE) {
        uint32 SID = (RXBnSIDH << 3) | (RXBnSIDL >> 5);
        uint32 EID = (RXBnSIDL & 3) << 16 | (RXBnEID8 << 8) | RXBnEID0;
        RecMsg->ID = SID << 18 | EID;
    } else {
        uint32 SID = (RXBnSIDH << 3) | (RXBnSIDL >> 5);
        RecMsg->ID = SID;
    }

    for (i = 0; i < RecMsg->DLC; i++) //��ȡ���յ�������
    {
        RecMsg->DATA[i] = MCP2515_ReadByte(RXB_CTRL_Address + 6 + i);
    }
}

void SetCfg(CanCfgStruct *CanCfg)
{
//     *(CanCfg->bitrate) = &bitrate_100Kbps[0];  // {CAN_100Kbps,PRSEG_8TQ,PHSEG1_8TQ,PHSEG2_3TQ,SJW_1TQ}
    CanCfg->bitrate[0] = bitrate_100Kbps[0];
    CanCfg->bitrate[1] = bitrate_100Kbps[1];
    CanCfg->bitrate[2] = bitrate_100Kbps[2];
    CanCfg->bitrate[3] = bitrate_100Kbps[3];
    CanCfg->bitrate[4] = bitrate_100Kbps[4];
    CanCfg->BUKT_enable = 1;
    CanCfg->CAN_MODE = 3;       // 000 = �趨Ϊ��������ģʽ
                                // 001 = �趨Ϊ����ģʽ
                                // 010 = �趨Ϊ����ģʽ
                                // 011 = �趨Ϊ������ģʽ
                                // 100 = �趨Ϊ����ģʽ
    CanCfg->CANINTE_enable = 3;
    CanCfg->CANINTF_enable = 0;

    CanCfg->RXM0ID = 0x1FFFFFFF;
    CanCfg->RXM1ID = 0x1FFFFFFF;

    CanCfg->RXF0ID = 0x100;
    CanCfg->RXF1ID = 0x7FE;
    CanCfg->RXF2ID = 0x101;
    CanCfg->RXF3ID = 0x102;
    CanCfg->RXF4ID = 0x103;
    CanCfg->RXF5ID = 0x104;

    CanCfg->RXF0IDE = 1;
    CanCfg->RXF1IDE = 0;
    CanCfg->RXF2IDE = 0;
    CanCfg->RXF3IDE = 1;
    CanCfg->RXF4IDE = 0;
    CanCfg->RXF5IDE = 1;
}

void ReadCfg(void) {
    printf("CNF1: %02bX ", MCP2515_ReadByte(CNF1));
    printf("CNF2: %02bX ", MCP2515_ReadByte(CNF2));
    printf("CNF3: %02bX \r\n", MCP2515_ReadByte(CNF3));
    printf("RXB0CTRL: %02bX ", MCP2515_ReadByte(RXB0CTRL));
    printf("CANINTF: %02bX ", MCP2515_ReadByte(CANINTF));
    printf("CANINTE: %02bX \r\n", MCP2515_ReadByte(CANINTE));
    printf("RXF0SIDH: %02bX ", MCP2515_ReadByte(RXF0SIDH));
    printf("RXF1SIDH: %02bX ", MCP2515_ReadByte(RXF1SIDH));
    printf("RXF2SIDH: %02bX \r\n", MCP2515_ReadByte(RXF2SIDH));
    printf("RXF3SIDH: %02bX ", MCP2515_ReadByte(RXF3SIDH));
    printf("RXF4SIDH: %02bX ", MCP2515_ReadByte(RXF4SIDH));
    printf("RXF5SIDH: %02bX  \r\n", MCP2515_ReadByte(RXF5SIDH));
    printf("RXM0SIDH: %02bX ", MCP2515_ReadByte(RXM0SIDH));
    printf("RXM1SIDH: %02bX ", MCP2515_ReadByte(RXM1SIDH));
    printf("CANCTRL: %02bX \r\n", MCP2515_ReadByte(CANCTRL));
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
    uint32 ID = 0x101;
    uint8 EXIDE = 0;
    uint8 DLC = 8;
    uint8 i;
    uint8 CANINTF_Flag;
    uint8 Send_data[] = {0x20, 0xF1, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

    MsgStruct SendMsg;
    MsgStruct RecMsg;

    CanCfgStruct CanCfg;
    SetCfg(&CanCfg);

    UART_init();    //UART1��ʼ������
    Exint_Init();            //�ⲿ�ж�1��ʼ������
//    MCP2515_Init(bitrate_100Kbps);

    Can_Init(&CanCfg);

//    ReadCfg();

    SendMsg.IsSend = 0x1;

    SendMsg.ID = ID;
    SendMsg.TYPE = 0x2;
    SendMsg.EXIDE = 0x0;
    SendMsg.DLC = 8;

    for (i = 0; i < 8; i++) //�����ַ�����ֱ������0�Ž���
    {
        SendMsg.DATA[i] = Send_data[i];
//        printf("SendMsg.DATA[%bd] = %bx \r\n", i, SendMsg.DATA[i]);
    }

    RecMsg.IsSend = 0;
//    Send(&SendMsg);

    for (i = 0; i < 2; i++) //�����ַ�����ֱ������0�Ž���
    {
        Send(&SendMsg);
        ShowMsg(&SendMsg);
        SendMsg.ID = 0x100;
        SendMsg.TYPE = 0x2;
        SendMsg.EXIDE = 0x1;
        SendMsg.DLC = 7;

        Delay_Nms(3000);

        printf("CAN_RX0IF_Flag = %bd \r\n", CAN_RX0IF_Flag);
        printf("CAN_RX1IF_Flag = %bd \r\n", CAN_RX1IF_Flag);
        printf("CANSTAT: %02bX \r\n", MCP2515_ReadByte(CANSTAT));


        CANINTF_Flag = MCP2515_ReadByte(CANINTF);
        printf("CANINTF: %02bX \r\n", CANINTF_Flag);

        if (CANINTF_Flag & RX0IF) {
            Receive(RXB0CTRL, &RecMsg);
            ShowMsg(&RecMsg);
            MCP2515_WriteByte(CANINTF, MCP2515_ReadByte(CANINTF) & 0xFE);//����жϱ�־λ(�жϱ�־�Ĵ���������MCU����)
        }

        if (CANINTF_Flag & RX1IF) {
            Receive(RXB1CTRL, &RecMsg);
            ShowMsg(&RecMsg);
            MCP2515_WriteByte(CANINTF, MCP2515_ReadByte(CANINTF) & 0xFD);//����жϱ�־λ(�жϱ�־�Ĵ���������MCU����)
        }
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