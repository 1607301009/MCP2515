/**********************************************************************************
 * 文件名  ：MCP2515.c
 * 描述    ：MCP2515驱动函数库         
 * 实验平台：NiRen_STC/IAP15核心板(或用户STC15单片机开发板) + NiRen_MCP2515 CAN模块    
**********************************************************************************/

#include <reg51.h>
#include "MCP2515.H"

//MCP2515引脚定义
sbit MCP2515_INT  = P1^6;//MCP2515中断引脚		（好像没有用到）

sbit MCP2515_SCK  = P1^5;//SPI时钟引脚 
sbit MCP2515_MOSI = P1^4;//SPI主机输出从机输入引脚 
sbit MCP2515_MISO = P1^3;//SPI主机输入从机输出引脚 
sbit MCP2515_CS   = P1^2;//SPI片选引脚

uint8 code TXB0_Address[]={TXB0CTRL,TXB0SIDH,TXB0SIDL,TXB0EID8,TXB0EID0,TXB0DLC,TXB0D0,TXB0D1,TXB0D2,TXB0D3,TXB0D4,TXB0D5,TXB0D6,TXB0D7};
uint8 code TXB1_Address[]={TXB1CTRL,TXB1SIDH,TXB1SIDL,TXB1EID8,TXB1EID0,TXB1DLC,TXB1D0,TXB1D1,TXB1D2,TXB1D3,TXB1D4,TXB1D5,TXB1D6,TXB1D7};
uint8 code TXB2_Address[]={TXB2CTRL,TXB2SIDH,TXB2SIDL,TXB2EID8,TXB2EID0,TXB2DLC,TXB2D0,TXB2D1,TXB2D2,TXB2D3,TXB2D4,TXB2D5,TXB2D6,TXB2D7};
 
uint8 code RXF0_Address[]={RXF0SIDH,RXF0SIDL,RXF0EID8,RXF0EID0};
uint8 code RXF1_Address[]={RXF1SIDH,RXF1SIDL,RXF1EID8,RXF1EID0};
uint8 code RXF2_Address[]={RXF2SIDH,RXF2SIDL,RXF2EID8,RXF2EID0};
uint8 code RXF3_Address[]={RXF3SIDH,RXF3SIDL,RXF3EID8,RXF3EID0};
uint8 code RXF4_Address[]={RXF4SIDH,RXF4SIDL,RXF4EID8,RXF4EID0};
uint8 code RXF5_Address[]={RXF5SIDH,RXF5SIDL,RXF5EID8,RXF5EID0};

uint8 code RXM0_Address[]={RXM0SIDH,RXM0SIDL,RXM0EID8,RXM0EID0};
uint8 code RXM1_Address[]={RXM1SIDH,RXM1SIDL,RXM1EID8,RXM1EID0};

/*******************************************************************************
* 函数名  : Delay_Nms
* 描述    : 通过软件延时约nms(不准确)
* 输入    : x
* 说明    : 此方式延时时间是不准确的,准确延时建议用定时器
*******************************************************************************/
void Delay_Nms(uint16 x)
{
	uint16 y;

	for (;x>0;x--)
		for (y=0;y<100;y++);
}


/*******************************************************************************
* 函数名  : SPI_ReadByte
* 描述    : 通过SPI读取一个字节数据
* 输入    : 无
* 输出    : 无
* 返回值  : rByte(读取到的一个字节数据)
* 说明    : 无
*******************************************************************************/
uint8 SPI_ReadByte(void)
{
	uint8 i,rByte=0;
	
	MCP2515_SCK=0;
	for(i=0;i<8;i++)
	{
		MCP2515_SCK=1;
		rByte<<=1;
		rByte|=MCP2515_MISO;
		MCP2515_SCK=0;	
	}
	return rByte;
}

/*******************************************************************************
* 函数名  : SPI_SendByte
* 描述    : SPI发送一个字节数据
* 输入    : dt:待发送的数据
* 输出    : 无
* 返回值  : 无
* 说明    : 无
*******************************************************************************/
void SPI_SendByte(uint8 dt)
{
	uint8 i;
		
	for(i=0;i<8;i++)
	{	
		MCP2515_SCK=0;
		if((dt<<i)&0x80)
			MCP2515_MOSI=1;
		else
			MCP2515_MOSI=0;					
		MCP2515_SCK=1;
	}
	MCP2515_SCK=0;
}

/*******************************************************************************
* 函数名  : MCP2515_WriteByte
* 描述    : 通过SPI向MCP2515指定地址寄存器写1个字节数据
* 输入    : addr:MCP2515寄存器地址,dat:待写入的数据
* 输出    : 无
* 返回值  : 无
* 说明    : 无
*******************************************************************************/
void MCP2515_WriteByte(uint8 addr,uint8 dat)
{
	MCP2515_CS=0;				//置MCP2515的CS为低电平
	SPI_SendByte(CAN_WRITE);	//发送写命令
	SPI_SendByte(addr);			//发送地址
	SPI_SendByte(dat);			//写入数据
	MCP2515_CS=1;				//置MCP2515的CS为高电平 
}

/*******************************************************************************
* 函数名  : MCP2515_ReadByte
* 描述    : 通过SPI从MCP2515指定地址寄器读1个字节数据
* 输入    : addr:MCP2515寄存器地址
* 输出    : 无
* 返回值  : rByte:读取到寄存器的1个字节数据
* 说明    : 无
*******************************************************************************/
uint8 MCP2515_ReadByte(uint8 addr)
{
	uint8 rByte;
	
	MCP2515_CS=0;				//置MCP2515的CS为低电平
	SPI_SendByte(CAN_READ);		//发送读命令
	SPI_SendByte(addr);			//发送地址
	rByte=SPI_ReadByte();		//读取数据
	MCP2515_CS=1;				//置MCP2515的CS为高电平
	return rByte;				//返回读到的一个字节数据
}

/*******************************************************************************
* 函数名  : MCP2515_Reset
* 描述    : 发送复位指令软件复位MCP2515
* 输入    : 无
* 输出    : 无
* 返回值  : 无
* 说明    : 将内部寄存器复位为缺省状态,并将器件设定为配置模式
*******************************************************************************/
void MCP2515_Reset(void)
{
	MCP2515_CS=0;				//置MCP2515的CS为低电平
	SPI_SendByte(CAN_RESET);	//发送寄存器复位命令
	MCP2515_CS=1;				//置MCP2515的CS为高电平
}

/*******************************************************************************
* 函数名  : CAN_Set_RX
* 描述    : 在配置模式下设置屏蔽器RXM和过滤器RXF
* 输入    : *CAN_TX_Buf(待发送数据缓冲区指针),len(待发送数据长度)
* 输出    : 无
* 返回值  : 无
* 说明    : 当ID>0x7FF设置设置为拓展帧
*******************************************************************************/
void CAN_Set_RX(uint8 RXF_Address,uint32 ID,uint8 EXIDE)
{
	//EXIDE=1、ID>0x7FF发送拓展帧
	if (ID<=0x7FF)
	{
		if (EXIDE)
		{
			MCP2515_WriteByte(RXF_Address+0,0x0);								  //SIDH
			MCP2515_WriteByte(RXF_Address+1,0x8);								  //SIDL
			MCP2515_WriteByte(RXF_Address+2,ID>>8&0xFF);						  //EID8
			MCP2515_WriteByte(RXF_Address+3,ID&0xFF);							  //EID0
		}																	  
		else
		{	
			MCP2515_WriteByte(RXF_Address+0,ID>>3);							  //SIDH
			MCP2515_WriteByte(RXF_Address+1,(ID&0x07)<<5);					  //SIDL
			MCP2515_WriteByte(RXF_Address+2,0);								  //EID8
			MCP2515_WriteByte(RXF_Address+3,0);								  //EID0
		}
	}
	else
	{	
		MCP2515_WriteByte(RXF_Address+0,ID>>21);								  //SIDH
		MCP2515_WriteByte(RXF_Address+1,(ID>>18&0x07)<<5|(ID>>16&0x03)|0x08);	  //SIDL
		MCP2515_WriteByte(RXF_Address+2,ID>>8&0xFF);							  //EID8
		MCP2515_WriteByte(RXF_Address+3,ID&0xFF);								  //EID0
	}
}

void Can_Init(CanCfgStruct *CanCfg)
{
    MCP2515_Reset();	//发送复位指令软件复位MCP2515
    Delay_Nms(1);		//通过软件延时约nms(不准确)

    //设置波特率
    //set CNF1,SJW=00,长度为1TQ,BRP=49,TQ=[2*(BRP+1)]/Fsoc=2*50/8M=12.5us
    MCP2515_WriteByte(CNF1,CanCfg->bitrate[4] | CanCfg->bitrate[0]);
    //set CNF2,SAM=0,在采样点对总线进行一次采样，PHSEG1=(2+1)TQ=3TQ,PRSEG=(0+1)TQ=1TQ
    MCP2515_WriteByte(CNF2,BTLMODE_CNF3 | CanCfg->bitrate[2] | CanCfg->bitrate[1]);
    //set CNF3,PHSEG2=(2+1)TQ=3TQ,同时当CANCTRL.CLKEN=1时设定CLKOUT引脚为时间输出使能位
    MCP2515_WriteByte(CNF3,SOF_ENABLED | CanCfg->bitrate[3]);

    MCP2515_WriteByte(RXB0CTRL,CanCfg->BUKT_enable << 2);   //如果RXB0满,RXB0 接收到的报文将被滚存至RXB1

    MCP2515_WriteByte(RXB1CTRL, RXM);   // 接收所有报文

    MCP2515_WriteByte(CANINTF,CanCfg->CANINTF_enable);//清空CAN中断标志寄存器的所有位(必须由MCU清空)
    MCP2515_WriteByte(CANINTE,CanCfg->CANINTE_enable);//配置CAN中断使能寄存器的接收缓冲器0满中断使能,其它位禁止中断

    CAN_Set_RX(RXF0SIDH, CanCfg->RXF0ID, CanCfg->RXF0IDE);
    CAN_Set_RX(RXF1SIDH, CanCfg->RXF1ID, CanCfg->RXF1IDE);
    CAN_Set_RX(RXF2SIDH, CanCfg->RXF2ID, CanCfg->RXF2IDE);
    CAN_Set_RX(RXF3SIDH, CanCfg->RXF3ID, CanCfg->RXF3IDE);
    CAN_Set_RX(RXF4SIDH, CanCfg->RXF4ID, CanCfg->RXF4IDE);
    CAN_Set_RX(RXF5SIDH, CanCfg->RXF5ID, CanCfg->RXF5IDE);

    CAN_Set_RX(RXM0SIDH, CanCfg->RXM0ID, 1);
    CAN_Set_RX(RXM1SIDH, CanCfg->RXM1ID, 1);

    MCP2515_WriteByte(CANCTRL,REQOP_LOOPBACK|CLKOUT_ENABLED);//将MCP2515设置为环回模式,退出配置模式
    if(OPMODE_NORMAL!=(MCP2515_ReadByte(CANSTAT)&&0xE0))//判断MCP2515是否已经进入环回模式
    {
        MCP2515_WriteByte(CANCTRL,REQOP_LOOPBACK|CLKOUT_ENABLED);//再次将MCP2515设置为环回模式,退出配置模式
    }
}

/*******************************************************************************
* 函数名  : MCP2515_Init
* 描述    : MCP2515初始化配置
* 输入    : 无
* 输出    : 无
* 返回值  : 无
* 说明    : 初始化包括：软件复位、工作波特率设置、标识符相关配置等。
*******************************************************************************/
void MCP2515_Init(uint8 *CAN_Bitrate)
{
	uint8 temp=0;

	MCP2515_Reset();	//发送复位指令软件复位MCP2515
	Delay_Nms(1);		//通过软件延时约nms(不准确)

	//设置波特率
	//set CNF1,SJW=00,长度为1TQ,BRP=49,TQ=[2*(BRP+1)]/Fsoc=2*50/8M=12.5us
	MCP2515_WriteByte(CNF1,CAN_Bitrate[4]|CAN_Bitrate[0]);
	//set CNF2,SAM=0,在采样点对总线进行一次采样，PHSEG1=(2+1)TQ=3TQ,PRSEG=(0+1)TQ=1TQ
	MCP2515_WriteByte(CNF2,BTLMODE_CNF3|CAN_Bitrate[2]|CAN_Bitrate[1]);
	//set CNF3,PHSEG2=(2+1)TQ=3TQ,同时当CANCTRL.CLKEN=1时设定CLKOUT引脚为时间输出使能位
	MCP2515_WriteByte(CNF3,SOF_ENABLED|CAN_Bitrate[3]);

	MCP2515_WriteByte(RXB0CTRL,0x06);//如果RXB0满,RXB0 接收到的报文将被滚存至RXB1

	//uint8 RXF_Address,uint32 ID,uint8 EXIDE)
	//RXB0 接收缓冲器配备有验收滤波寄存器RXF0 和RXF1（以及过滤屏蔽寄存器RXM0）
	CAN_Set_RX(RXF0SIDH,0x100,1);
	CAN_Set_RX(RXF1SIDH,0x7FE,0);

	CAN_Set_RX(RXM0SIDH,0x7FF,0);

	//RXB1 配备有验收滤波寄存器RXF2、RXF3、RXF4、RXF5和滤波屏蔽寄存器RXM1。
	CAN_Set_RX(RXF2SIDH,0x800,1);
	CAN_Set_RX(RXF3SIDH,0x1FFFFFFF,1);
	CAN_Set_RX(RXF4SIDH,0x7FF,0);
	CAN_Set_RX(RXF5SIDH,0x0,0);
	
	CAN_Set_RX(RXM1SIDH,0x1FFFFFFE,0);

//	MCP2515_WriteByte(TXB0SIDH,0xAB);//发送缓冲器0标准标识符高位
//	MCP2515_WriteByte(TXB0SIDL,0xE0);//|0x08|0x2);//发送缓冲器0标准标识符低位
//	MCP2515_WriteByte(TXB0EID8,0x00);//发送缓冲器0标准标识符高位
//	MCP2515_WriteByte(TXB0EID0,0x00);//发送缓冲器0标准标识符低位
//
//
//	MCP2515_WriteByte(RXB0SIDH,0x00);//清空接收缓冲器0的标准标识符高位
//	MCP2515_WriteByte(RXB0SIDL,0x00);//清空接收缓冲器0的标准标识符低位
//
//	MCP2515_WriteByte(RXB0CTRL,0x62);//仅仅接收标准标识符的有效信息
//	MCP2515_WriteByte(RXB0DLC,DLC_8);//设置接收数据的长度为8个字节
//	
//	MCP2515_WriteByte(RXF0SIDH,0xFF);       //配置验收滤波寄存器n标准标识符高位
//
//	MCP2515_WriteByte(RXM0SIDH,0xFF);       //配置验收屏蔽寄存器n标准标识符高位
//	MCP2515_WriteByte(RXM1EID0,0xE0);       //配置验收屏蔽寄存器n标准标识符低位
//		
	MCP2515_WriteByte(CANINTF,0x00);    //清空CAN中断标志寄存器的所有位(必须由MCU清空)
	MCP2515_WriteByte(CANINTE,0x03);    //配置CAN中断使能寄存器的接收缓冲器满中断使能,其它位禁止中断
	
//	MCP2515_WriteByte(CANCTRL,REQOP_LOOPBACK|CLKOUT_ENABLED);//|OSM_ENABLED将MCP2515设置为正常模式,退出配置模式
//
//	temp=MCP2515_ReadByte(CANSTAT);//读取CAN状态寄存器的值
//	if(OPMODE_LOOPBACK!=(temp&&0xE0))//判断MCP2515是否已经进入正常模式
//	{
//	    MCP2515_WriteByte(CANCTRL,REQOP_LOOPBACK|CLKOUT_ENABLED);//|OSM_ENABLED再次将MCP2515设置为正常模式,退出配置模式
//	}

    MCP2515_WriteByte(CANCTRL,REQOP_LOOPBACK|CLKOUT_ENABLED);//将MCP2515设置为环回模式,退出配置模式

    temp=MCP2515_ReadByte(CANSTAT);//读取CAN状态寄存器的值
    if(OPMODE_NORMAL!=(temp&&0xE0))//判断MCP2515是否已经进入环回模式
    {
    MCP2515_WriteByte(CANCTRL,REQOP_LOOPBACK|CLKOUT_ENABLED);//再次将MCP2515设置为环回模式,退出配置模式
    }

}

/*******************************************************************************
* 函数名  : CAN_Send_Buffer
* 描述    : CAN发送指定长度的数据
* 输入    : *CAN_TX_Buf(待发送数据缓冲区指针),len(待发送数据长度)
* 输出    : 无
* 返回值  : 无
* 说明    :
* 改进点   ：在报文发送之前， MCU 应对CANINTE.TXInE 位进行初始化，以便在报文发送时使能或禁止中断的产生
 *          在写入发送缓冲器之前，必须将TXBnCTRL.TXREQ 位清零（表明发送缓冲器无等待发送的报文）。
*******************************************************************************/
void CAN_Send_buffer(uint32 ID,uint8 EXIDE,uint8 DLC,uint8 *Send_data)
{
	uint8 TXBnCTRL;         // 选择发送缓冲器

   	uint8 i;
	//寄存器状态获取TXB0是否忙碌，=1为忙碌，=0为空闲
	uint8 Read_TXBnCTRL = MCP2515_ReadByte(TXB0CTRL);
	if ((Read_TXBnCTRL & TXREQ) == 0)
	{
	    TXBnCTRL = TXB0CTRL;
	}
	else
	{
	    Read_TXBnCTRL = MCP2515_ReadByte(TXB1CTRL);
	    if ((Read_TXBnCTRL & TXREQ) == 0)
	    {
	        TXBnCTRL = TXB1CTRL;
	    }
	    else
	    {
	        Read_TXBnCTRL = MCP2515_ReadByte(TXB2CTRL);  // 如果前面两个寄存器全部忙碌， 默认使用第3个
	        TXBnCTRL = TXB2CTRL;
	    }
	}

	// 设置DLC， 默认为数据帧 RTR=0   后期开发远程帧
	MCP2515_WriteByte(TXBnCTRL + 5, DLC);

	// EXIDE=1、ID>0x7FF发送拓展帧
	if (ID<=0x7FF)
	{
		if (EXIDE)
		{
		    MCP2515_WriteByte(TXBnCTRL + 1, 0x0);			//SIDH
		    MCP2515_WriteByte(TXBnCTRL + 2, 0x8);			//SIDL
		    MCP2515_WriteByte(TXBnCTRL + 3, ID>>8&0xFF);	//EID8
            MCP2515_WriteByte(TXBnCTRL + 4, ID&0xFF);		//EID0
		}																	  
		else
		{	
		    MCP2515_WriteByte(TXBnCTRL + 1, ID>>3);		  //SIDH
		    MCP2515_WriteByte(TXBnCTRL + 2, (ID&0x07)<<5);  //SIDL
		    MCP2515_WriteByte(TXBnCTRL + 3, 0);			  //EID8
		    MCP2515_WriteByte(TXBnCTRL + 4, 0);			  //EID0
		}
	}
	else
	{	
	    MCP2515_WriteByte(TXBnCTRL + 1, ID>>21);								  //SIDH
	    MCP2515_WriteByte(TXBnCTRL + 2, (ID>>18&0x07)<<5|(ID>>16&0x03)|0x08);	  //SIDL
	    MCP2515_WriteByte(TXBnCTRL + 3, ID>>8&0xFF);							  //EID8
	    MCP2515_WriteByte(TXBnCTRL + 4, ID&0xFF);								  //EID0
	}
	
	for(i = 0;(i <= DLC && i <= 8); i++)
	{
	    MCP2515_WriteByte(TXBnCTRL + 6 + i, Send_data[i]);						  //D0_8将待发送的数据写入发送缓冲寄存器
	}

	MCP2515_CS=0;
	MCP2515_WriteByte(TXBnCTRL, Read_TXBnCTRL | TXREQ);	//CTRL 开始发送
	MCP2515_CS=1;
	//发送形式设置：单次发送，远程帧，拓展帧，调试状态，回应。

    // 等待报文发送成功
    for(i = 0; i < 14; i++)
    {
        if ((MCP2515_ReadByte(TXBnCTRL) & TXREQ) == 0)
        {
            return;
        }
        Delay_Nms(100);
    }
}

/*******************************************************************************
* 函数名  : CAN_Receive_Buffer
* 描述    : CAN接收一帧数据
* 输入    : RX_Address(需要读取的RXB0CTRL、RXB1CTRL地址),*CAN_TX_Buf(待接收数据缓冲区指针)
* 输出    : 无
* 返回值  : len(接收到数据的长度,0~8字节)
* 说明    : 无
*******************************************************************************/
void CAN_Receive_Buffer(uint8 RXB_CTRL_Address,uint8 *CAN_RX_Buf)
{	
	uint8 j;
	for(j=0;j<14;j++)
	{
		CAN_RX_Buf[j]=MCP2515_ReadByte(RXB_CTRL_Address+j);//把CAN接收到的数据放入指定缓冲区
		//CAN_RX_Buf[j]=RXB_CTRL_Address+j;
	}
	if (RXB_CTRL_Address==RXB0CTRL)
		{
		MCP2515_WriteByte(CANINTF,0);//清除中断标志位(中断标志寄存器必须由MCU清零)
		}
	else if (RXB_CTRL_Address==RXB1CTRL)
		{
		MCP2515_WriteByte(CANINTF,0);//清除中断标志位(中断标志寄存器必须由MCU清零)	
		}
}


