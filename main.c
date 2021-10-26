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


char putchar(char c)  //printf���������putchar()
{
    UART_send_str(c);
    return c;
}

////MCP2515������	Ҫ����FOSC=8M BRP=0..64 PRSEG=1..8 PHSEG1=3..16 PHSEG2=2..8 SJW=1..4
//uint8 code bitrate_5Kbps[5] = {CAN_5Kbps,PRSEG_6TQ,PHSEG1_7TQ,PHSEG2_2TQ,SJW_1TQ};
//uint8 code bitrate_10Kbps[5] = {CAN_10Kbps,PRSEG_6TQ,PHSEG1_7TQ,PHSEG2_2TQ,SJW_1TQ};
//uint8 code bitrate_25Kbps[5] = {CAN_25Kbps,PRSEG_6TQ,PHSEG1_7TQ,PHSEG2_2TQ,SJW_1TQ};
//uint8 code bitrate_50Kbps[5] = {CAN_50Kbps,PRSEG_6TQ,PHSEG1_7TQ,PHSEG2_2TQ,SJW_1TQ};
//uint8 code bitrate_100Kbps[5] = {CAN_100Kbps,PRSEG_8TQ,PHSEG1_8TQ,PHSEG2_3TQ,SJW_1TQ};
//uint8 code bitrate_125Kbps[5] = {CAN_125Kbps,PRSEG_6TQ,PHSEG1_7TQ,PHSEG2_2TQ,SJW_1TQ};
//uint8 code bitrate_250Kbps[5] = {CAN_250Kbps,PRSEG_6TQ,PHSEG1_7TQ,PHSEG2_2TQ,SJW_1TQ};
//uint8 code bitrate_500Kbps[5] = {CAN_500Kbps,PRSEG_2TQ,PHSEG1_3TQ,PHSEG2_2TQ,SJW_1TQ};


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

//    if (Flag&0x80) CAN_MERRF_Flag = 1;                            //CAN���Ĵ����жϱ�־λ
//    if (Flag&0x40) CAN_WAKIF_Flag = 1;                            //CAN�����жϱ�־λ
//    if (Flag&0x20) CAN_ERRIF_Flag = 1;                            //CAN�����жϱ�־λ��EFLG �Ĵ������ж���ж�Դ��
//    if (Flag&0x10) CAN_TX2IF_Flag = 1;                            //MCP2515���ͻ�����2 ���жϱ�־λ
//    if (Flag&0x08) CAN_TX1IF_Flag = 1;                            //MCP2515���ͻ�����1 ���жϱ�־λ
//    if (Flag&0x04) CAN_TX0IF_Flag = 1;                            //MCP2515���ͻ�����0 ���жϱ�־λ
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

// ���ñ�����
void SetBitrate(uint8 _5Kbps, uint8 *bitrate){
    uint8 kbps, prseg, phseg1, phseg2, sjw;
    //MCP2515������	Ҫ����FOSC=8M BRP=0..64 PRSEG=1..8 PHSEG1=3..16 PHSEG2=2..8 SJW=1..4
    //    uint8 bitrate_5Kbps[5] = {CAN_5Kbps,PRSEG_6TQ,PHSEG1_7TQ,PHSEG2_2TQ,SJW_1TQ};
    //    uint8 bitrate_10Kbps[5] = {CAN_10Kbps,PRSEG_6TQ,PHSEG1_7TQ,PHSEG2_2TQ,SJW_1TQ};
    //    uint8 bitrate_25Kbps[5] = {CAN_25Kbps,PRSEG_6TQ,PHSEG1_7TQ,PHSEG2_2TQ,SJW_1TQ};
    //    uint8 bitrate_50Kbps[5] = {CAN_50Kbps,PRSEG_6TQ,PHSEG1_7TQ,PHSEG2_2TQ,SJW_1TQ};
    //    uint8 bitrate_100Kbps[5] = {CAN_100Kbps,PRSEG_8TQ,PHSEG1_8TQ,PHSEG2_3TQ,SJW_1TQ};
    //    uint8 bitrate_125Kbps[5] = {CAN_125Kbps,PRSEG_6TQ,PHSEG1_7TQ,PHSEG2_2TQ,SJW_1TQ};
    //    uint8 bitrate_250Kbps[5] = {CAN_250Kbps,PRSEG_6TQ,PHSEG1_7TQ,PHSEG2_2TQ,SJW_1TQ};
    //    uint8 bitrate_500Kbps[5] = {CAN_500Kbps,PRSEG_2TQ,PHSEG1_3TQ,PHSEG2_2TQ,SJW_1TQ};
    switch (_5Kbps * 5) {
        case 5:
            kbps = CAN_5Kbps, prseg = PRSEG_6TQ, phseg1 = PHSEG1_7TQ, phseg2 = PHSEG2_2TQ, sjw = SJW_1TQ;
            break; /* ��ѡ�� */
        case 10:
            kbps = CAN_10Kbps, prseg = PRSEG_6TQ, phseg1 = PHSEG1_7TQ, phseg2 = PHSEG2_2TQ, sjw = SJW_1TQ;
            break; /* ��ѡ�� */
        case 25:
            kbps = CAN_25Kbps, prseg = PRSEG_6TQ, phseg1 = PHSEG1_7TQ, phseg2 = PHSEG2_2TQ, sjw = SJW_1TQ;
            break; /* ��ѡ�� */
        case 50:
            kbps = CAN_50Kbps, prseg = PRSEG_6TQ, phseg1 = PHSEG1_7TQ, phseg2 = PHSEG2_2TQ, sjw = SJW_1TQ;
            break; /* ��ѡ�� */
        case 100:
            kbps = CAN_100Kbps, prseg = PRSEG_8TQ, phseg1 = PHSEG1_8TQ, phseg2 = PHSEG2_3TQ, sjw = SJW_1TQ;
            break; /* ��ѡ�� */
        case 125:
            kbps = CAN_125Kbps, prseg = PRSEG_6TQ, phseg1 = PHSEG1_7TQ, phseg2 = PHSEG2_2TQ, sjw = SJW_1TQ;
            break; /* ��ѡ�� */
        case 250:
            kbps = CAN_250Kbps, prseg = PRSEG_6TQ, phseg1 = PHSEG1_7TQ, phseg2 = PHSEG2_2TQ, sjw = SJW_1TQ;
            break; /* ��ѡ�� */

        default : /* ��ѡ�� */
            kbps = CAN_500Kbps, prseg = PRSEG_2TQ, phseg1 = PHSEG1_3TQ, phseg2 = PHSEG2_2TQ, sjw = SJW_1TQ;
    }
    bitrate[0] = kbps, bitrate[1] = prseg, bitrate[2] = phseg1, bitrate[3] = phseg2, bitrate[4] = sjw;
}

void printE2Write(uint8 *E2_data, uint8 add, uint8 Len) {
    uint8 i;
    printf("add: %02bX Len: %02bX Data:", add, Len);
    for (i = 0; i < Len; i++) //�����ַ�����ֱ������0�Ž���
    {
        printf(" %02bX", E2_data[i]);
    }
    printf("\r\n");
}


void SaveCfgToE2(CanCfgStruct *CanCfg) {
    uint8 E2_data[8];
//    uint8 read_data[8];

    E2_data[E2_5Kbps] = CanCfg->_5Kbps;
    E2_data[E2_BUKT_enable] = CanCfg->BUKT_enable;
    E2_data[E2_RXB0RXM] = CanCfg->RXB0RXM;
    E2_data[E2_RXB1RXM] = CanCfg->RXB1RXM;
    E2_data[E2_CAN_MODE] = CanCfg->CAN_MODE;
    E2_data[E2_CANINTE_enable] = CanCfg->CANINTE_enable;
    E2_data[E2_CANINTF_enable] = CanCfg->CANINTF_enable;
    E2Write(E2_data, E2_CanCifg, 7);
//    printE2Write(E2_data, E2_CanCifg, 7);

    //    ����������0
    E2_data[0] = CanCfg->RXM0ID >> 24;
    E2_data[1] = (CanCfg->RXM0ID >> 16) & 0xFF;
    E2_data[2] = (CanCfg->RXM0ID >> 8) & 0xFF;
    E2_data[3] = CanCfg->RXM0ID & 0xFF;
    //    ����������1
    E2_data[4] = CanCfg->RXM0ID >> 24;
    E2_data[5] = (CanCfg->RXM0ID >> 16) & 0xFF;
    E2_data[6] = (CanCfg->RXM0ID >> 8) & 0xFF;
    E2_data[7] = CanCfg->RXM0ID & 0xFF;
    E2Write(E2_data, E2_RXM01ID, 8);
//    printE2Write(E2_data, E2_RXM01ID, 8);

    // �˲���0��1�� ��λΪ��չ֡��־λ
    E2_data[0] = CanCfg->RXF0ID >> 24 | (CanCfg->RXF0IDE << 7);
    E2_data[1] = (CanCfg->RXF0ID >> 16) & 0xFF;
    E2_data[2] = (CanCfg->RXF0ID >> 8) & 0xFF;
    E2_data[3] = CanCfg->RXF0ID & 0xFF;
    E2_data[4] = (CanCfg->RXF1ID >> 24) | (CanCfg->RXF1IDE << 7);
    E2_data[5] = (CanCfg->RXF1ID >> 16) & 0xFF;
    E2_data[6] = (CanCfg->RXF1ID >> 8) & 0xFF;
    E2_data[7] = CanCfg->RXF1ID & 0xFF;
    E2Write(E2_data, E2_RXF01, 8);
//    printE2Write(E2_data, E2_RXF01, 8);

    // �˲���2��3
    E2_data[0] = CanCfg->RXF2ID >> 24 | (CanCfg->RXF2IDE << 7);
    E2_data[1] = (CanCfg->RXF2ID >> 16) & 0xFF;
    E2_data[2] = (CanCfg->RXF2ID >> 8) & 0xFF;
    E2_data[3] = CanCfg->RXF2ID & 0xFF;
    E2_data[4] = (CanCfg->RXF3ID >> 24) | (CanCfg->RXF3IDE << 7);
    E2_data[5] = (CanCfg->RXF3ID >> 16) & 0xFF;
    E2_data[6] = (CanCfg->RXF3ID >> 8) & 0xFF;
    E2_data[7] = CanCfg->RXF3ID & 0xFF;
    E2Write(E2_data, E2_RXF23, 8);
//    printE2Write(E2_data, E2_RXF23, 8);

    // �˲���4��5
    E2_data[0] = CanCfg->RXF4ID >> 24 | (CanCfg->RXF4IDE << 7);
    E2_data[1] = (CanCfg->RXF4ID >> 16) & 0xFF;
    E2_data[2] = (CanCfg->RXF4ID >> 8) & 0xFF;
    E2_data[3] = CanCfg->RXF4ID & 0xFF;
    E2_data[4] = (CanCfg->RXF5ID >> 24) | (CanCfg->RXF5IDE << 7);
    E2_data[5] = (CanCfg->RXF5ID >> 16) & 0xFF;
    E2_data[6] = (CanCfg->RXF5ID >> 8) & 0xFF;
    E2_data[7] = CanCfg->RXF5ID & 0xFF;
    E2Write(E2_data, E2_RXF45, 8);
//    printE2Write(E2_data, E2_RXF45, 8);
}

void PrintfCfg(CanCfgStruct *CanCfg) {
//    printf("_5Kbps: %02bX \r\n", CanCfg->_5Kbps);
//    printf("bitrate[0]: %02bX \r\n", CanCfg->bitrate[0]);
//    printf("bitrate[1]: %02bX \r\n", CanCfg->bitrate[1]);
//    printf("bitrate[2]: %02bX \r\n", CanCfg->bitrate[2]);
//    printf("bitrate[3]: %02bX \r\n", CanCfg->bitrate[3]);
//    printf("bitrate[4]: %02bX \r\n", CanCfg->bitrate[4]);
//    printf("BUKT_enable: %02bX \r\n", CanCfg->BUKT_enable);
//    printf("CAN_MODE: %02bX \r\n", CanCfg->CAN_MODE);

    printf("CANINTE: %02bX \r\n", CanCfg->CANINTE_enable);
    printf("CANINTF: %02bX \r\n", CanCfg->CANINTF_enable);

    printf("RXM0ID: %08lX \r\n", CanCfg->RXM0ID);
    printf("RXM1ID: %08lX \r\n", CanCfg->RXM1ID);
//    printf("RXF0ID: %07lX \r\n", CanCfg->RXF0ID);
//    printf("RXF1ID: %07lX \r\n", CanCfg->RXF1ID);
//    printf("RXF2ID: %07lX \r\n", CanCfg->RXF2ID);
//    printf("RXF3ID: %07lX \r\n", CanCfg->RXF3ID);
//    printf("RXF4ID: %07lX \r\n", CanCfg->RXF4ID);
//    printf("RXF5ID: %07lX \r\n", CanCfg->RXF5ID);
//
//    printf("RXF0IDE: %bX \r\n", CanCfg->RXF0IDE);
//    printf("RXF1IDE: %bX \r\n", CanCfg->RXF1IDE);
//    printf("RXF2IDE: %bX \r\n", CanCfg->RXF2IDE);
//    printf("RXF3IDE: %bX \r\n", CanCfg->RXF3IDE);
//    printf("RXF4IDE: %bX \r\n", CanCfg->RXF4IDE);
//    printf("RXF5IDE: %bX \r\n", CanCfg->RXF5IDE);
}

void SetCfgFromE2(CanCfgStruct *CanCfg) {
    uint8 i;
    uint8 E2_read_data[8];

    for (i = 0; i < 5; i++) //�����ַ�����ֱ������0�Ž���
    {
        E2Read(E2_read_data, i * 8, sizeof(E2_read_data));
        printf("SetCfgFromE2 ");
        printE2Write(E2_read_data, i * 8, sizeof(E2_read_data));
    }

    //  ���ò�����
    E2Read(E2_read_data, E2_CanCifg, sizeof(E2_read_data));  // �� EEPROM ��ȡһ������
    CanCfg->_5Kbps = E2_read_data[E2_5Kbps];
    SetBitrate(CanCfg->_5Kbps, &(CanCfg->bitrate));
    //  ���ù���ʹ��λ������ģʽ���ж�ʹ��λ���жϱ�־λ
    CanCfg->BUKT_enable = E2_read_data[E2_BUKT_enable];
    CanCfg->CAN_MODE = E2_read_data[E2_CAN_MODE];      // 0:���� 1:���� 2:���� 3:���� 4:����
    CanCfg->CANINTE_enable = E2_read_data[E2_CANINTE_enable];
    CanCfg->CANINTF_enable = E2_read_data[E2_CANINTF_enable];

    //  ����������
    E2Read(E2_read_data, E2_RXM01ID, sizeof(E2_read_data));  // �� EEPROM ��ȡһ������
    CanCfg->RXM0ID = (E2_read_data[0] << 24) | (E2_read_data[1] << 16) | (E2_read_data[2] << 8) | E2_read_data[3];
    CanCfg->RXM1ID = E2_read_data[4] << 24 | E2_read_data[5] << 16 | E2_read_data[6] << 8 | E2_read_data[7];
//    CanCfg->RXM0ID = (E2_read_data[0] & 0x7F) << 24 | E2_read_data[1] << 16 | E2_read_data[2] << 8 | E2_read_data[3];
//    CanCfg->RXM1ID = (E2_read_data[4] & 0x7F) << 24 | E2_read_data[5] << 16 | E2_read_data[6] << 8 | E2_read_data[7];
printf("data[0]: %02bX %lX\r\n", E2_read_data[0], E2_read_data[0] << 4 | 0x10);
if (CanCfg->RXM0ID == 0xffffffff) {
    printf("- %04bX\r\n", E2_read_data[4]);
} else {
    printf("+ %lX\r\n", CanCfg->RXM0ID);
}
//printf("data[4]: %07lX \r\n", E2_read_data[4] << 24);
    //  �˲���0��1
    E2Read(E2_read_data, E2_RXF01, sizeof(E2_read_data));  // �� EEPROM ��ȡһ������
    CanCfg->RXF0IDE = E2_read_data[0] >> 7;
    CanCfg->RXF0ID = (E2_read_data[0] & 0x7F) << 24 | E2_read_data[1] << 16 | E2_read_data[2] << 8 | E2_read_data[3];
    CanCfg->RXF1IDE = E2_read_data[4] >> 7;
    CanCfg->RXF1ID = (E2_read_data[4] & 0x7F) << 24 | E2_read_data[5] << 16 | E2_read_data[6] << 8 | E2_read_data[7];
    //  �˲���0��3
    E2Read(E2_read_data, E2_RXF23, sizeof(E2_read_data));  // �� EEPROM ��ȡһ������
    CanCfg->RXF2IDE = E2_read_data[0] >> 7;
    CanCfg->RXF2ID = (E2_read_data[0] & 0x7F) << 24 | E2_read_data[1] << 16 | E2_read_data[2] << 8 | E2_read_data[3];
    CanCfg->RXF3IDE = E2_read_data[4] >> 7;
    CanCfg->RXF3ID = (E2_read_data[4] & 0x7F) << 24 | E2_read_data[5] << 16 | E2_read_data[6] << 8 | E2_read_data[7];
    //  �˲���0��5
    E2Read(E2_read_data, E2_RXF45, sizeof(E2_read_data));  // �� EEPROM ��ȡһ������
    CanCfg->RXF4IDE = E2_read_data[0] >> 7;
    CanCfg->RXF4ID = (E2_read_data[0] & 0x7F) << 24 | E2_read_data[1] << 16 | E2_read_data[2] << 8 | E2_read_data[3];
    CanCfg->RXF5IDE = E2_read_data[4] >> 7;
    CanCfg->RXF5ID = (E2_read_data[4] & 0x7F) << 24 | E2_read_data[5] << 16 | E2_read_data[6] << 8 | E2_read_data[7];
}

void SetCfg(CanCfgStruct *CanCfg)
{
    CanCfg->_5Kbps = 20;
//    {CAN_100Kbps,PRSEG_8TQ,PHSEG1_8TQ,PHSEG2_3TQ,SJW_1TQ}
    CanCfg->bitrate[0] = CAN_100Kbps;
    CanCfg->bitrate[1] = PRSEG_8TQ;
    CanCfg->bitrate[2] = PHSEG1_8TQ;
    CanCfg->bitrate[3] = PHSEG2_3TQ;
    CanCfg->bitrate[4] = SJW_1TQ;
//    CanCfg->bitrate[0] = bitrate_100Kbps[0];
//    CanCfg->bitrate[1] = bitrate_100Kbps[1];
//    CanCfg->bitrate[2] = bitrate_100Kbps[2];
//    CanCfg->bitrate[3] = bitrate_100Kbps[3];
//    CanCfg->bitrate[4] = bitrate_100Kbps[4];
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
    uint8 Send_data[] = {0x20, 0xF1, 0x03, 0x03, 0x04, 0x05, 0x06, 0x07};
    uint8 E2_data[8];

    MsgStruct SendMsg;
    MsgStruct RecMsg;

    CanCfgStruct CanCfg;

    UART_init();    //UART1��ʼ������
    Exint_Init();            //�ⲿ�ж�1��ʼ������
//    MCP2515_Init(bitrate_100Kbps);

    SetCfg(&CanCfg);
    PrintfCfg(&CanCfg);

    SaveCfgToE2(&CanCfg);

    SetCfgFromE2(&CanCfg);
    PrintfCfg(&CanCfg);

    Can_Init(&CanCfg);

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

//        printf("CAN_RX0IF_Flag = %bd \r\n", CAN_RX0IF_Flag);
//        printf("CAN_RX1IF_Flag = %bd \r\n", CAN_RX1IF_Flag);
//        printf("CANSTAT: %02bX \r\n", MCP2515_ReadByte(CANSTAT));


        CANINTF_Flag = MCP2515_ReadByte(CANINTF);
//        printf("CANINTF: %02bX \r\n", CANINTF_Flag);

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