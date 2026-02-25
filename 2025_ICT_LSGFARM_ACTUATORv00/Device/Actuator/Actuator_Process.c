/*
 * GreenHouseActNodeV3
 *
 *
 * Actuator_Process.c
 *
 *  Created on: 2024. 8. 20.
 *      Author: CoreSolution
 *
 * 2024.8.20 수정 구동기상태 수정
 * 2024.5.9 수정  타이머 전용에서 타이머ON과 강제ON 용으로 확장 수정중
 * 2024.3.29 수정  타이머 전용에서 타이머ON과 강제ON 용으로 확장 수정중

 */

#include "Actuator_Process.h"
#include "../Master/Master_Process.h"
#include "tick_timer.h"

uint16_t RelayStatus;
uint8_t CRC1, CRC2;
uint8_t MstCRC1, MstCRC2;
uint16_t crc_table[256] = {
	0x0000,0xC0C1,0xC181,0x0140,0xC301,0x03C0,0x0280,0xC241,0xC601,0x06C0,0x0780,0xC741,0x0500,0xC5C1,
	0xC481,0x0440,0xCC01,0x0CC0,0x0D80,0xCD41,0x0F00,0xCFC1,0xCE81,0x0E40,0x0A00,0xCAC1,0xCB81,0x0B40,
	0xC901,0x09C0,0x0880,0xC841,0xD801,0x18C0,0x1980,0xD941,0x1B00,0xDBC1,0xDA81,0x1A40,0x1E00,0xDEC1,
	0xDF81,0x1F40,0xDD01,0x1DC0,0x1C80,0xDC41,0x1400,0xD4C1,0xD581,0x1540,0xD701,0x17C0,0x1680,0xD641,
	0xD201,0x12C0,0x1380,0xD341,0x1100,0xD1C1,0xD081,0x1040,0xF001,0x30C0,0x3180,0xF141,0x3300,0xF3C1,
	0xF281,0x3240,0x3600,0xF6C1,0xF781,0x3740,0xF501,0x35C0,0x3480,0xF441,0x3C00,0xFCC1,0xFD81,0x3D40,
	0xFF01,0x3FC0,0x3E80,0xFE41,0xFA01,0x3AC0,0x3B80,0xFB41,0x3900,0xF9C1,0xF881,0x3840,0x2800,0xE8C1,
	0xE981,0x2940,0xEB01,0x2BC0,0x2A80,0xEA41,0xEE01,0x2EC0,0x2F80,0xEF41,0x2D00,0xEDC1,0xEC81,0x2C40,
	0xE401,0x24C0,0x2580,0xE541,0x2700,0xE7C1,0xE681,0x2640,0x2200,0xE2C1,0xE381,0x2340,0xE101,0x21C0,
	0x2080,0xE041,0xA001,0x60C0,0x6180,0xA141,0x6300,0xA3C1,0xA281,0x6240,0x6600,0xA6C1,0xA781,0x6740,
	0xA501,0x65C0,0x6480,0xA441,0x6C00,0xACC1,0xAD81,0x6D40,0xAF01,0x6FC0,0x6E80,0xAE41,0xAA01,0x6AC0,
	0x6B80,0xAB41,0x6900,0xA9C1,0xA881,0x6840,0x7800,0xB8C1,0xB981,0x7940,0xBB01,0x7BC0,0x7A80,0xBA41,
	0xBE01,0x7EC0,0x7F80,0xBF41,0x7D00,0xBDC1,0xBC81,0x7C40,0xB401,0x74C0,0x7580,0xB541,0x7700,0xB7C1,
	0xB681,0x7640,0x7200,0xB2C1,0xB381,0x7340,0xB101,0x71C0,0x7080,0xB041,0x5000,0x90C1,0x9181,0x5140,
	0x9301,0x53C0,0x5280,0x9241,0x9601,0x56C0,0x5780,0x9741,0x5500,0x95C1,0x9481,0x5440,0x9C01,0x5CC0,
	0x5D80,0x9D41,0x5F00,0x9FC1,0x9E81,0x5E40,0x5A00,0x9AC1,0x9B81,0x5B40,0x9901,0x59C0,0x5880,0x9841,
	0x8801,0x48C0,0x4980,0x8941,0x4B00,0x8BC1,0x8A81,0x4A40,0x4E00,0x8EC1,0x8F81,0x4F40,0x8D01,0x4DC0,
	0x4C80,0x8C41,0x4400,0x84C1,0x8581,0x4540,0x8701,0x47C0,0x4680,0x8641,0x8201,0x42C0,0x4380,0x8341,
	0x4100,0x81C1,0x8081,0x4040 };

/********************************************
CRC Check
********************************************/
uint16_t Crc_Check(volatile uint8_t *nData, uint16_t wLength)
{
	uint8_t  nTemp;
	uint16_t wCRCWord = 0xFFFF;

	while (wLength--)
	{
		nTemp = *nData++ ^ wCRCWord;
		wCRCWord >>= 8;
		wCRCWord ^= crc_table[nTemp];
	}

	CRC1 = (wCRCWord & 0xFF);
	CRC2 = wCRCWord >> 8;

	return wCRCWord;
}

/********************************************
CRC Check
********************************************/
uint16_t Master_Crc_Check(volatile uint8_t *nData, uint16_t wLength)
{
	uint8_t  nTemp;
	uint16_t wCRCWord = 0xFFFF;

	while (wLength--)
	{
		nTemp = *nData++ ^ wCRCWord;
		wCRCWord >>= 8;
		wCRCWord ^= crc_table[nTemp];
	}

	MstCRC1 = (wCRCWord & 0xFF);
	MstCRC2 = wCRCWord >> 8;

	return wCRCWord;
}

void Actuator_RS485_Push_Data(uint8_t nData)
{
	// Ori source code
//	Actuator_Ring_Buffer.RxBuffer[Actuator_Ring_Buffer.nSaveIndex++] = nData;
//
//    if(Actuator_Ring_Buffer.nSaveIndex >= UART_RX_RING_BUFFER_SIZE)
//    {
//    	Actuator_Ring_Buffer.nSaveIndex = 0;
//    }

	// 25.12.31 Fixed by diskept
	uint16_t next = (uint16_t)(Actuator_Ring_Buffer.nSaveIndex + 1U);

	if(next >= UART_RX_RING_BUFFER_SIZE)
	{
		next = 0;
	}

	// Buffer full: drop oldest byte to keep receiver alive
	if(next == Actuator_Ring_Buffer.nProcessIndex)
	{
		Actuator_Ring_Buffer.nProcessIndex++;
		if(Actuator_Ring_Buffer.nProcessIndex >= UART_RX_RING_BUFFER_SIZE)
		{
			Actuator_Ring_Buffer.nProcessIndex = 0;
		}
		Actuator_Ring_Buffer.nRxOverflowCnt++;
	}

	Actuator_Ring_Buffer.RxBuffer[Actuator_Ring_Buffer.nSaveIndex] = nData;
	Actuator_Ring_Buffer.nSaveIndex = next;
}

uint8_t Actuator_RS485_Pop_Data(void)
{
    uint8_t nRet;

    nRet = Actuator_Ring_Buffer.RxBuffer[Actuator_Ring_Buffer.nProcessIndex++];

    if(Actuator_Ring_Buffer.nProcessIndex >= UART_RX_RING_BUFFER_SIZE)
    {
    	Actuator_Ring_Buffer.nProcessIndex = 0;
    }
    return nRet;
}

uint16_t Actuator_Get_NodeID(void)
{
	uint16_t SenNodeID = 0;

	SenNodeID = (NODE_ID1) ? 0 : 1;
	SenNodeID += (NODE_ID2) ? 0 : 2;
	SenNodeID += (NODE_ID3) ? 0 : 4;
	SenNodeID += (NODE_ID4) ? 0 : 8;
	SenNodeID += (NODE_ID5) ? 0 : 16;

	return SenNodeID;
}

void Actuator_Install_Initialize(void)
{
	//========================================================================================
	Actuator.SW_Relay16ch = INSTALLED;
	Actuator.OCS_Relay16ch = INSTALLED;
	Actuator.OCS_Relay16ch_EXT = INSTALLED;

	Actuator.Switch_Num1 = INSTALLED;
	Actuator.Switch_Num2 = INSTALLED;
	Actuator.Switch_Num3 = INSTALLED;
	Actuator.Switch_Num4 = INSTALLED;
	Actuator.Switch_Num5 = INSTALLED;
	Actuator.Switch_Num6 = INSTALLED;
	Actuator.Switch_Num7 = INSTALLED;
	Actuator.Switch_Num8 = INSTALLED;
	Actuator.Switch_Num9 = INSTALLED;
	Actuator.Switch_Num10 = INSTALLED;
	Actuator.Switch_Num11 = INSTALLED;
	Actuator.Switch_Num12 = INSTALLED;
	Actuator.Switch_Num13 = INSTALLED;
	Actuator.Switch_Num14 = INSTALLED;
	Actuator.Switch_Num15 = INSTALLED;
	Actuator.Switch_Num16 = INSTALLED;

	ActNode.NodeReg[SWITCH_1_ADDR] = SWITCH;
	ActNode.NodeReg[SWITCH_2_ADDR] = SWITCH;
	ActNode.NodeReg[SWITCH_3_ADDR] = SWITCH;
	ActNode.NodeReg[SWITCH_4_ADDR] = SWITCH;
	ActNode.NodeReg[SWITCH_5_ADDR] = SWITCH;
	ActNode.NodeReg[SWITCH_6_ADDR] = SWITCH;
	ActNode.NodeReg[SWITCH_7_ADDR] = SWITCH;
	ActNode.NodeReg[SWITCH_8_ADDR] = SWITCH;
	ActNode.NodeReg[SWITCH_9_ADDR] = SWITCH;
	ActNode.NodeReg[SWITCH_10_ADDR] = SWITCH;
	ActNode.NodeReg[SWITCH_11_ADDR] = SWITCH;
	ActNode.NodeReg[SWITCH_12_ADDR] = SWITCH;
	ActNode.NodeReg[SWITCH_13_ADDR] = SWITCH;
	ActNode.NodeReg[SWITCH_14_ADDR] = SWITCH;
	ActNode.NodeReg[SWITCH_15_ADDR] = SWITCH;
	ActNode.NodeReg[SWITCH_16_ADDR] = SWITCH;

	Actuator.OCSwitch_Num1 = INSTALLED;
	Actuator.OCSwitch_Num2 = INSTALLED;
	Actuator.OCSwitch_Num3 = INSTALLED;
	Actuator.OCSwitch_Num4 = INSTALLED;
	Actuator.OCSwitch_Num5 = INSTALLED;
	Actuator.OCSwitch_Num6 = INSTALLED;
	Actuator.OCSwitch_Num7 = INSTALLED;
	Actuator.OCSwitch_Num8 = INSTALLED;
	Actuator.OCSwitch_Num9 = NOTINSTALLED;
	Actuator.OCSwitch_Num10 = NOTINSTALLED;
	Actuator.OCSwitch_Num11 = NOTINSTALLED;
	Actuator.OCSwitch_Num12 = NOTINSTALLED;
	Actuator.OCSwitch_Num13 = NOTINSTALLED;
	Actuator.OCSwitch_Num14 = NOTINSTALLED;
	Actuator.OCSwitch_Num15 = NOTINSTALLED;
	Actuator.OCSwitch_Num16 = NOTINSTALLED;
	if(NodeVersion != KSX_TEST)
	{
		Actuator.OCSwitch_Num9 = INSTALLED;
		Actuator.OCSwitch_Num10 = INSTALLED;
		Actuator.OCSwitch_Num11 = INSTALLED;
		Actuator.OCSwitch_Num12 = INSTALLED;
		Actuator.OCSwitch_Num13 = INSTALLED;
		Actuator.OCSwitch_Num14 = INSTALLED;
		Actuator.OCSwitch_Num15 = INSTALLED;
		Actuator.OCSwitch_Num16 = INSTALLED;
	}

	ActNode.NodeReg[OCSWITCH_1_ADDR] = OCSWITCH;
	ActNode.NodeReg[OCSWITCH_2_ADDR] = OCSWITCH;
	ActNode.NodeReg[OCSWITCH_3_ADDR] = OCSWITCH;
	ActNode.NodeReg[OCSWITCH_4_ADDR] = OCSWITCH;
	ActNode.NodeReg[OCSWITCH_5_ADDR] = OCSWITCH;
	ActNode.NodeReg[OCSWITCH_6_ADDR] = OCSWITCH;
	ActNode.NodeReg[OCSWITCH_7_ADDR] = OCSWITCH;
	ActNode.NodeReg[OCSWITCH_8_ADDR] = OCSWITCH;
	ActNode.NodeReg[OCSWITCH_9_ADDR] = OCSWITCH;
	ActNode.NodeReg[OCSWITCH_10_ADDR] = OCSWITCH;
	ActNode.NodeReg[OCSWITCH_11_ADDR] = OCSWITCH;
	ActNode.NodeReg[OCSWITCH_12_ADDR] = OCSWITCH;
	ActNode.NodeReg[OCSWITCH_13_ADDR] = OCSWITCH;
	ActNode.NodeReg[OCSWITCH_14_ADDR] = OCSWITCH;
	ActNode.NodeReg[OCSWITCH_15_ADDR] = OCSWITCH;
	ActNode.NodeReg[OCSWITCH_16_ADDR] = OCSWITCH;

//========================================================================================
	OCS_Control[OCS_NUM1].mTMR_1Sec = sec_timer_create();
	OCS_Control[OCS_NUM2].mTMR_1Sec = sec_timer_create();
	OCS_Control[OCS_NUM3].mTMR_1Sec = sec_timer_create();
	OCS_Control[OCS_NUM4].mTMR_1Sec = sec_timer_create();
	OCS_Control[OCS_NUM5].mTMR_1Sec = sec_timer_create();
	OCS_Control[OCS_NUM6].mTMR_1Sec = sec_timer_create();
	OCS_Control[OCS_NUM7].mTMR_1Sec = sec_timer_create();
	OCS_Control[OCS_NUM8].mTMR_1Sec = sec_timer_create();
	OCS_Control[OCS_NUM9].mTMR_1Sec = sec_timer_create();
	OCS_Control[OCS_NUM10].mTMR_1Sec = sec_timer_create();
	OCS_Control[OCS_NUM11].mTMR_1Sec = sec_timer_create();
	OCS_Control[OCS_NUM12].mTMR_1Sec = sec_timer_create();
	OCS_Control[OCS_NUM13].mTMR_1Sec = sec_timer_create();
	OCS_Control[OCS_NUM14].mTMR_1Sec = sec_timer_create();
	OCS_Control[OCS_NUM15].mTMR_1Sec = sec_timer_create();
	OCS_Control[OCS_NUM16].mTMR_1Sec = sec_timer_create();

	SW_Control[SW_NUM1].mTMR_1Sec = sec_timer_create();
	SW_Control[SW_NUM2].mTMR_1Sec = sec_timer_create();
	SW_Control[SW_NUM3].mTMR_1Sec = sec_timer_create();
	SW_Control[SW_NUM4].mTMR_1Sec = sec_timer_create();
	SW_Control[SW_NUM5].mTMR_1Sec = sec_timer_create();
	SW_Control[SW_NUM6].mTMR_1Sec = sec_timer_create();
	SW_Control[SW_NUM7].mTMR_1Sec = sec_timer_create();
	SW_Control[SW_NUM8].mTMR_1Sec = sec_timer_create();
	SW_Control[SW_NUM9].mTMR_1Sec = sec_timer_create();
	SW_Control[SW_NUM10].mTMR_1Sec = sec_timer_create();
	SW_Control[SW_NUM11].mTMR_1Sec = sec_timer_create();
	SW_Control[SW_NUM12].mTMR_1Sec = sec_timer_create();
	SW_Control[SW_NUM13].mTMR_1Sec = sec_timer_create();
	SW_Control[SW_NUM14].mTMR_1Sec = sec_timer_create();
	SW_Control[SW_NUM15].mTMR_1Sec = sec_timer_create();
	SW_Control[SW_NUM16].mTMR_1Sec = sec_timer_create();

	Debug_printf(" Operate_Config %04X %04X\n",
			  Operate_Config.nNode485Baud, Operate_Config.nAct485Baud);
}

//===================================================================================
//ID 03 00 00 00 01 xx xx
//ID 03 02 00 55 xx xx
void Actuator_OCS_Relay_Get_Tx_Data(void)	//  Relay Status Get
{
	uint8_t	nIndex = 0;

	if(NodeVersion == RELAY_32CH_VER)
	{
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = OCS_RELAY16CH_ID;		// Slave ID (board #2)
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x03;					// Modbus Read Holding Registers (0x03)

		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x00;					// Start Address = 0x0000
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x00;

		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x00;					// Register Qty = 0x0002 (=> 4 bytes)
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x02;

		Crc_Check((volatile uint8_t *)Actuator_Ring_Buffer.TxBuffer, nIndex);
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = CRC1;
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = CRC2;

		Actuator_Ring_Buffer.nSaveCnt = 9;
		Actuator_Ring_Buffer.Acutator = ACTUATOR_OCS_RELAYCH_READ;
	}
	else
	{
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = OCS_RELAY16CH_ID;		// Slave Address
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x03;    				// Modbus Read Input Registers Command

		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x00;    				// Start Address
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x00;

		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x00;    				// Length(2Byte)
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x01;

		Crc_Check((volatile uint8_t *)Actuator_Ring_Buffer.TxBuffer, nIndex);
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = CRC1;
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = CRC2;

		Actuator_Ring_Buffer.nSaveCnt = 7;
		Actuator_Ring_Buffer.Acutator = ACTUATOR_OCS_RELAYCH_READ;
	}

	ACT_TX_Enable;
	ACT_LED_ON;
	HAL_UART_Transmit(&huart3, Actuator_Ring_Buffer.TxBuffer, nIndex, 1000);
	ACT_LED_OFF;
	ACT_RX_Enable;
}

//ID 0F 00 00 00 08 01 55 xx xx
//ID 0F 00 00 00 08 xx xx
void Actuator_OCS_Relay_Set_Tx_Data(uint32_t wRelayValue)	//  Relay Status Set
{
	uint8_t	nIndex = 0;
	uint8_t	lowByte, highByte;

	uint32_t mask = Actuator_OCS_Control.nWriteRelay;				// 32ch 마스크는 구조체에서 읽음

	Actuator_Ring_Buffer.TxBuffer[nIndex++] = OCS_RELAY16CH_ID;    	// Slave Address
	Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x0F;    				// Modbus Write Multiple Coils Command

	Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x00;    				// Start Address
	Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x00;

	if(NodeVersion == RELAY_32CH_VER)
	{
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x00;
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x20;

		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x04; 			// Length(2Byte)

		// Coil data: LSB-first per byte (CH1..8 → byte0 bit0..7)
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = (uint8_t)((mask >> 0)  & 0xFF);
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = (uint8_t)((mask >> 8)  & 0xFF);
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = (uint8_t)((mask >> 16) & 0xFF);
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = (uint8_t)((mask >> 24) & 0xFF);
	}
	else
	{
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x00;
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x10;

		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x02; 			// Length(2Byte)

		if (RelayVersion == Rly_Old)
		{
			highByte = (uint8_t)(wRelayValue >> 8);  // version 1 릴레이모듈 프로토콜 비트
			lowByte = (uint8_t)(wRelayValue);        // version 1 릴레이모듈 프로토콜 비트
		}
		else if(RelayVersion == Rly_New)
		{
			lowByte= (uint8_t)(wRelayValue >> 8);   //버전 2릴레이모듈 프로토콜 비트
			highByte = (uint8_t)(wRelayValue);      //버전 2릴레이모듈 프로토콜 비트
		}
		else
		{
			highByte = (uint8_t)(wRelayValue >> 8);  // version 1 릴레이모듈 프로토콜 비트
			lowByte = (uint8_t)(wRelayValue);        // version 1 릴레이모듈 프로토콜 비트
		}

//		highByte = (uint8_t)(wRelayValue >> 8);   	//버전 1릴레이모듈 프로토콜 비트
//		lowByte = (uint8_t)(wRelayValue);   		//버전 1릴레이모듈 프로토콜 비트
//		lowByte= (uint8_t)(wRelayValue >> 8);	   	//버전 2릴레이모듈 프로토콜 비트
//		highByte = (uint8_t)(wRelayValue);      	//버전 2릴레이모듈 프로토콜 비트

		Actuator_Ring_Buffer.TxBuffer[nIndex++] = highByte;
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = lowByte;
	}


	Crc_Check((volatile uint8_t *)Actuator_Ring_Buffer.TxBuffer, nIndex);

	Actuator_Ring_Buffer.TxBuffer[nIndex++] = CRC1;
	Actuator_Ring_Buffer.TxBuffer[nIndex++] = CRC2;

	Actuator_Ring_Buffer.nSaveCnt = 8;
	Actuator_Ring_Buffer.Acutator = ACTUATOR_OCS_RELAY16CH_WRITE;

	ACT_TX_Enable;
	ACT_LED_ON;
	HAL_UART_Transmit(&huart3, Actuator_Ring_Buffer.TxBuffer, nIndex, 1000);
	ACT_LED_OFF;
	ACT_RX_Enable;
}

//===================================================================================
//ID 03 00 00 00 01 xx xx
//ID 03 02 00 55 xx xx
void Actuator_OCS_EXT_Relay_Get_Tx_Data(void)	//  Relay Status Get
{
	uint8_t	nIndex = 0;

	if(NodeVersion == RELAY_32CH_VER)
	{
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = OCS_RELAY16CH_EXT_ID;	// Slave ID (board #2)
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x03;					// Modbus Read Holding Registers (0x03)

		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x00;					// Start Address = 0x0000
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x00;

		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x00;					// Register Qty = 0x0002 (=> 4 bytes)
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x02;

		Crc_Check((volatile uint8_t *)Actuator_Ring_Buffer.TxBuffer, nIndex);
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = CRC1;
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = CRC2;

		Actuator_Ring_Buffer.nSaveCnt = 9;
		Actuator_Ring_Buffer.Acutator = ACTUATOR_OCS_EXT_RELAYCH_READ;
	}
	else
	{
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = OCS_RELAY16CH_EXT_ID;	// Slave Address
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x03;    				// Modbus Read Input Registers Command

		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x00;    				// Start Address
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x00;

		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x00;    				// Length(2Byte)
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x01;

		Crc_Check((volatile uint8_t *)Actuator_Ring_Buffer.TxBuffer, nIndex);
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = CRC1;
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = CRC2;

		Actuator_Ring_Buffer.nSaveCnt = 7;
		Actuator_Ring_Buffer.Acutator = ACTUATOR_OCS_EXT_RELAYCH_READ;
	}

	ACT_TX_Enable;
	ACT_LED_ON;
	HAL_UART_Transmit(&huart3, Actuator_Ring_Buffer.TxBuffer, nIndex, 1000);
	ACT_LED_OFF;
	ACT_RX_Enable;
}

//ID 0F 00 00 00 08 01 55 xx xx
//ID 0F 00 00 00 08 xx xx
void Actuator_OCS_EXT_Relay_Set_Tx_Data(uint32_t wRelayValue)	//  Relay Status Set
{
	uint8_t	nIndex = 0;
	uint8_t	lowByte, highByte;

	uint32_t mask = Actuator_OCS_EXT_Control.nWriteRelay;				// 32ch 마스크는 구조체에서 읽음

	Actuator_Ring_Buffer.TxBuffer[nIndex++] = OCS_RELAY16CH_EXT_ID; // Slave Address
	Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x0F;    				// Modbus Write Multiple Coils Command

	Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x00;    				// Start Address
	Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x00;

	if (NodeVersion == RELAY_32CH_VER)
	{
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x00;
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x20;

		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x04; 			// Length(2Byte)

		// Coil data: LSB-first per byte (CH1..8 → byte0 bit0..7)
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = (uint8_t)((mask >> 0)  & 0xFF);
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = (uint8_t)((mask >> 8)  & 0xFF);
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = (uint8_t)((mask >> 16) & 0xFF);
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = (uint8_t)((mask >> 24) & 0xFF);

	}
	else
	{
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x00;
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x10;

		Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x02; 			// Length(2Byte)

		if (RelayVersion == Rly_Old)
		{
			highByte = (uint8_t)(wRelayValue >> 8);  // version 1 릴레이모듈 프로토콜 비트
			lowByte = (uint8_t)(wRelayValue);        // version 1 릴레이모듈 프로토콜 비트
		}
		else if(RelayVersion == Rly_New)
		{
			lowByte= (uint8_t)(wRelayValue >> 8);   //버전 2릴레이모듈 프로토콜 비트
			highByte = (uint8_t)(wRelayValue);      //버전 2릴레이모듈 프로토콜 비트
		}
		else
		{
			highByte = (uint8_t)(wRelayValue >> 8);  // version 1 릴레이모듈 프로토콜 비트
			lowByte = (uint8_t)(wRelayValue);        // version 1 릴레이모듈 프로토콜 비트
		}

//		highByte = (uint8_t)(wRelayValue >> 8);   	//버전 1릴레이모듈 프로토콜 비트
//		lowByte = (uint8_t)(wRelayValue);   		//버전 1릴레이모듈 프로토콜 비트
//		lowByte= (uint8_t)(wRelayValue >> 8);	   	//버전 2릴레이모듈 프로토콜 비트
//		highByte = (uint8_t)(wRelayValue);      	//버전 2릴레이모듈 프로토콜 비트

		Actuator_Ring_Buffer.TxBuffer[nIndex++] = highByte;
		Actuator_Ring_Buffer.TxBuffer[nIndex++] = lowByte;
	}

	Crc_Check((volatile uint8_t *)Actuator_Ring_Buffer.TxBuffer, nIndex);

	Actuator_Ring_Buffer.TxBuffer[nIndex++] = CRC1;
	Actuator_Ring_Buffer.TxBuffer[nIndex++] = CRC2;

	Actuator_Ring_Buffer.nSaveCnt = 8;
	Actuator_Ring_Buffer.Acutator = ACTUATOR_OCS_EXT_RELAY16CH_WRITE;

	ACT_TX_Enable;
	ACT_LED_ON;
	HAL_UART_Transmit(&huart3, Actuator_Ring_Buffer.TxBuffer, nIndex, 1000);
	ACT_LED_OFF;
	ACT_RX_Enable;
}

//ID 03 00 00 00 01 xx xx
//ID 03 02 00 55 xx xx
void Actuator_SW_Relay_Get_Tx_Data(void)	//  Relay Status Get
{
	uint8_t	nIndex = 0;

	Actuator_Ring_Buffer.TxBuffer[nIndex++] = SW_RELAY16CH_ID;    	// Slave Address
	Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x03;    				// Modbus Read Input Registers Command

	Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x00;    // Start Address
	Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x00;

	Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x00;    // Length(2Byte)
	Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x01;

	Crc_Check((volatile uint8_t *)Actuator_Ring_Buffer.TxBuffer, nIndex);
	Actuator_Ring_Buffer.TxBuffer[nIndex++] = CRC1;
	Actuator_Ring_Buffer.TxBuffer[nIndex++] = CRC2;

	Actuator_Ring_Buffer.nSaveCnt = 7;
	Actuator_Ring_Buffer.Acutator = ACTUATOR_SW_RELAYCH_READ;

	ACT_TX_Enable;
	ACT_LED_ON;
		HAL_UART_Transmit(&huart3, Actuator_Ring_Buffer.TxBuffer, nIndex, 1000);
	ACT_LED_OFF;
	ACT_RX_Enable;
}

//ID 0F 00 00 00 08 01 55 xx xx
//ID 0F 00 00 00 08 xx xx
void Actuator_SW_Relay_Set_Tx_Data(uint16_t wRelayValue)	//  Relay Status Set
{
	uint8_t	nIndex = 0;
	uint8_t	lowByte, highByte;

	Actuator_Ring_Buffer.TxBuffer[nIndex++] = SW_RELAY16CH_ID;    	// Slave Address
	Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x0F;    	// Modbus Write Multiple Coils Command

	Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x00;    	// Start Address
	Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x00;

	Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x00;
	Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x10;

	Actuator_Ring_Buffer.TxBuffer[nIndex++] = 0x02; 	// Length(2Byte)


	if (RelayVersion == Rly_Old)
	{
		highByte = (uint8_t)(wRelayValue >> 8);  // version 1 릴레이모듈 프로토콜 비트
		lowByte = (uint8_t)(wRelayValue);        // version 1 릴레이모듈 프로토콜 비트
	}
	else
	{
		lowByte= (uint8_t)(wRelayValue >> 8);   //버전 2릴레이모듈 프로토콜 비트
		highByte = (uint8_t)(wRelayValue);      //버전 2릴레이모듈 프로토콜 비트
	}

//	highByte = (uint8_t)(wRelayValue >> 8);  // version 1 릴레이모듈 프로토콜 비트
//	lowByte = (uint8_t)(wRelayValue);        // version 1 릴레이모듈 프로토콜 비트

//	lowByte= (uint8_t)(wRelayValue >> 8);   //버전 2릴레이모듈 프로토콜 비트
//	highByte = (uint8_t)(wRelayValue);      //버전 2릴레이모듈 프로토콜 비트

	Actuator_Ring_Buffer.TxBuffer[nIndex++] = highByte;
	Actuator_Ring_Buffer.TxBuffer[nIndex++] = lowByte;

	Crc_Check((volatile uint8_t *)Actuator_Ring_Buffer.TxBuffer, nIndex);
	Actuator_Ring_Buffer.TxBuffer[nIndex++] = CRC1;
	Actuator_Ring_Buffer.TxBuffer[nIndex++] = CRC2;

	Actuator_Ring_Buffer.nSaveCnt = 8;
	Actuator_Ring_Buffer.Acutator = ACTUATOR_SW_RELAY16CH_WRITE;

	ACT_TX_Enable;
	ACT_LED_ON;
	HAL_UART_Transmit(&huart3, Actuator_Ring_Buffer.TxBuffer, nIndex, 1000);
	ACT_LED_OFF;
	ACT_RX_Enable;
}

void Actuator_Buffer_Process(void)
{
    static uint8_t RxBuffer[UART_RX_RING_BUFFER_SIZE];
    static uint8_t nIndex = 0;

	if(Actuator_Ring_Buffer.nSaveIndex != Actuator_Ring_Buffer.nProcessIndex)
	{
        uint8_t RxData = Actuator_RS485_Pop_Data();

#if ACTUATOR_DEBUG
        Debug_printf("%02x", RxData);
#endif

        if(Actuator_Ring_Buffer.Acutator == ACTUATOR_OCS_RELAYCH_READ)
        {
    		if((RxData == OCS_RELAY16CH_ID) && (nIndex == 0))
    		{
    			RxBuffer[nIndex++] = RxData;
    		}
    		else if(nIndex == (Actuator_Ring_Buffer.nSaveCnt - 1))
    		{
    			RxBuffer[nIndex++] = RxData;
    			RxBuffer[nIndex] = '\0';
    			Actuator_Packet_Process(RxBuffer, nIndex);
                nIndex = 0;
    		}
    		else if((nIndex >= 1) && (nIndex < Actuator_Ring_Buffer.nSaveCnt))
    		{
    			RxBuffer[nIndex++] = RxData;
    		}
    		else
    		{
    			nIndex = 0;
    		}
    	}
        else if(Actuator_Ring_Buffer.Acutator == ACTUATOR_OCS_RELAY16CH_WRITE)
        {
    		if(RxData == OCS_RELAY16CH_ID && nIndex == 0)
    		{
    			RxBuffer[nIndex++] = RxData;
    		}
    		else if(nIndex == (Actuator_Ring_Buffer.nSaveCnt - 1))
    		{
    			RxBuffer[nIndex++] = RxData;
    			RxBuffer[nIndex] = '\0';
    			Actuator_Packet_Process(RxBuffer, nIndex);
                nIndex = 0;
    		}
    		else if((nIndex >= 1) && (nIndex < Actuator_Ring_Buffer.nSaveCnt))
    		{
    			RxBuffer[nIndex++] = RxData;
    		}
    		else
    		{
    			nIndex = 0;
    		}
    	}
//===================================================================================
        else if(Actuator_Ring_Buffer.Acutator == ACTUATOR_OCS_EXT_RELAYCH_READ)
        {
    		if((RxData == OCS_RELAY16CH_EXT_ID) && (nIndex == 0))
    		{
    			RxBuffer[nIndex++] = RxData;
    		}
    		else if(nIndex == (Actuator_Ring_Buffer.nSaveCnt - 1))
    		{
    			RxBuffer[nIndex++] = RxData;
    			RxBuffer[nIndex] = '\0';
    			Actuator_Packet_Process(RxBuffer, nIndex);
                nIndex = 0;
    		}
    		else if ((nIndex >= 1) && (nIndex < Actuator_Ring_Buffer.nSaveCnt))
    		{
    			RxBuffer[nIndex++] = RxData;
    		}
    		else
    		{
    			nIndex = 0;
    		}
    	}
        else if(Actuator_Ring_Buffer.Acutator ==  ACTUATOR_OCS_EXT_RELAY16CH_WRITE)
        {
    		if(RxData == OCS_RELAY16CH_EXT_ID && nIndex == 0)
    		{
    			RxBuffer[nIndex++] = RxData;
    		}
    		else if(nIndex == (Actuator_Ring_Buffer.nSaveCnt - 1))
    		{
    			RxBuffer[nIndex++] = RxData;
    			RxBuffer[nIndex] = '\0';
    			Actuator_Packet_Process(RxBuffer, nIndex);
                nIndex = 0;
    		}
    		else if((nIndex >= 1) && (nIndex < Actuator_Ring_Buffer.nSaveCnt))
    		{
    			RxBuffer[nIndex++] = RxData;
    		}
    		else
    		{
    			nIndex = 0;
    		}
    	}
//===================================================================================
        else if(Actuator_Ring_Buffer.Acutator ==  ACTUATOR_SW_RELAYCH_READ)
        {
    		if((RxData == SW_RELAY16CH_ID) && nIndex == 0)
    		{
    			RxBuffer[nIndex++] = RxData;
    		}
    		else if(nIndex == (Actuator_Ring_Buffer.nSaveCnt - 1))
    		{
    			RxBuffer[nIndex++] = RxData;
    			RxBuffer[nIndex] = '\0';
    			Actuator_Packet_Process(RxBuffer, nIndex);
                nIndex = 0;
    		}
    		else if((nIndex >= 1) && (nIndex < Actuator_Ring_Buffer.nSaveCnt))
    		{
    			RxBuffer[nIndex++] = RxData;
    		}
    		else
    		{
    			nIndex = 0;
    		}
    	}
        else if(Actuator_Ring_Buffer.Acutator ==  ACTUATOR_SW_RELAY16CH_WRITE)
        {
    		if(RxData == SW_RELAY16CH_ID && nIndex == 0)
    		{
    			RxBuffer[nIndex++] = RxData;
    		}
    		else if(nIndex == (Actuator_Ring_Buffer.nSaveCnt - 1))
    		{
    			RxBuffer[nIndex++] = RxData;
    			RxBuffer[nIndex] = '\0';
    			Actuator_Packet_Process(RxBuffer, nIndex);
                nIndex = 0;
    		}
    		else if((nIndex >= 1) && nIndex < Actuator_Ring_Buffer.nSaveCnt)
    		{
    			RxBuffer[nIndex++] = RxData;
    		}
    		else
    		{
    			nIndex = 0;
    		}
    	}
	}
}

void Actuator_Packet_Process(uint8_t* pPacket, uint8_t nSize)
{
#if ACTUATOR_DEBUG
	uint8_t cnt;

	for(cnt = 0; cnt < nSize; cnt++)
	{
		Debug_printf("\n%d : %02x",cnt, pPacket[cnt]);
	}
#endif

	if(nSize > 2)
		Crc_Check((volatile uint8_t *)pPacket, nSize-2);
	else
		return;

	if((CRC1 == pPacket[nSize - 2]) && (CRC2 == pPacket[nSize - 1]))
	{
		if(pPacket[0] == OCS_RELAY16CH_ID)
		{
			if(Actuator_Ring_Buffer.Acutator == ACTUATOR_OCS_RELAYCH_READ)
			{
				if(NodeVersion == RELAY_32CH_VER)
				{
					/* v3(32ch): ByteCount = 0x04 → 32비트 조립 (LSB-first per byte) */
                    if (pPacket[1] == 0x03 && pPacket[2] == 0x04)
                    {
                    	uint16_t regLo = (uint16_t)(((uint16_t)pPacket[3] << 8) | pPacket[4]);		// Addr 0x0000 → CH1..16
						uint16_t regHi = (uint16_t)(((uint16_t)pPacket[5] << 8) | pPacket[6]);		// Addr 0x0001 → CH17..32
						Actuator_OCS_Control.nReadRelay = ((uint32_t)regHi << 16) | regLo;  		// [HI:CH17..32][LO:CH1..16]
                        Actuator_OCS_Control.State = ACTUATOR_RECV_OK;
                    }
                    else
                    {
                        Actuator_OCS_Control.State = ACTUATOR_RECV_ERROR;
                    }
				}
				else
				{
					if(pPacket[1] == 0x03 && pPacket[2] == 0x02)
					{
						Actuator_OCS_Control.nReadRelay =  (uint16_t)(((uint16_t)pPacket[3] << 8) + pPacket[4]);
						Actuator_OCS_Control.State = ACTUATOR_RECV_OK;
					}
					else
					{
						Actuator_OCS_Control.State = ACTUATOR_RECV_ERROR;
					}
				}
			}
			else if(Actuator_Ring_Buffer.Acutator == ACTUATOR_OCS_RELAY16CH_WRITE)
			{
				if(NodeVersion == RELAY_32CH_VER)
				{
					if(pPacket[1] == 0x0F && pPacket[5] == 0x20)
					{
						Actuator_OCS_Control.State = ACTUATOR_SEND_OK;
					}
					else
					{
						Actuator_OCS_Control.State = ACTUATOR_SEND_ERROR;
					}
				}
				else
				{
					if(pPacket[1] == 0x0F && pPacket[5] == 0x10)
					{
						Actuator_OCS_Control.State = ACTUATOR_SEND_OK;
					}
					else
					{
						Actuator_OCS_Control.State = ACTUATOR_SEND_ERROR;
					}
				}
			}
		}
		//===============================================================================================
		else if(pPacket[0] == OCS_RELAY16CH_EXT_ID)
		{
			if(Actuator_Ring_Buffer.Acutator == ACTUATOR_OCS_EXT_RELAYCH_READ)
			{
				if(NodeVersion == RELAY_32CH_VER)
				{
					/* v3(32ch): ByteCount = 0x04 → 32비트 조립 (LSB-first per byte) */
                    if (pPacket[1] == 0x03 && pPacket[2] == 0x04)
                    {
                    	uint16_t regLo = (uint16_t)(((uint16_t)pPacket[3] << 8) | pPacket[4]);			// Addr 0x0000 → CH1..16
						uint16_t regHi = (uint16_t)(((uint16_t)pPacket[5] << 8) | pPacket[6]);			// Addr 0x0001 → CH17..32
						Actuator_OCS_EXT_Control.nReadRelay = ((uint32_t)regHi << 16) | regLo;  		// [HI:CH17..32][LO:CH1..16]
                        Actuator_OCS_EXT_Control.State = ACTUATOR_RECV_OK;
                    }
                    else {
                    	Actuator_OCS_EXT_Control.State = ACTUATOR_RECV_ERROR;
                    }
				}
				else
				{
					if(pPacket[1] == 0x03 && pPacket[2] == 0x02)
					{
						Actuator_OCS_EXT_Control.nReadRelay =  (uint16_t)(((uint16_t)pPacket[3] << 8) + pPacket[4]);
						Actuator_OCS_EXT_Control.State = ACTUATOR_RECV_OK;
					}
					else
					{
						Actuator_OCS_EXT_Control.State = ACTUATOR_RECV_ERROR;
					}
				}
			}
			else if(Actuator_Ring_Buffer.Acutator ==  ACTUATOR_OCS_EXT_RELAY16CH_WRITE)
			{
				if(NodeVersion == RELAY_32CH_VER)
				{
					if(pPacket[1] == 0x0F && pPacket[5] == 0x20)
					{
						Actuator_OCS_EXT_Control.State = ACTUATOR_SEND_OK;
					}
					else
					{
						Actuator_OCS_EXT_Control.State = ACTUATOR_SEND_ERROR;
					}
				}
				else
				{
					if(pPacket[1] == 0x0F && pPacket[5] == 0x10)
					{
						Actuator_OCS_EXT_Control.State = ACTUATOR_SEND_OK;
					}
					else
					{
						Actuator_OCS_EXT_Control.State = ACTUATOR_SEND_ERROR;
					}
				}
			}
		}
		//===============================================================================================
		else if(pPacket[0] == SW_RELAY16CH_ID)
		{
			if(Actuator_Ring_Buffer.Acutator ==  ACTUATOR_SW_RELAYCH_READ)
			{
				if(pPacket[1] == 0x03 && pPacket[2] == 0x02)
				{
					Actuator_SW_Control.nReadRelay =  (uint16_t)(((uint16_t)pPacket[3] << 8) + pPacket[4]);
					RelayStatus = Actuator_SW_Control.nReadRelay;
					Actuator_SW_Control.State = ACTUATOR_RECV_OK;
				}
				else
				{
					Actuator_SW_Control.State = ACTUATOR_RECV_ERROR;
				}
			}
			else if(Actuator_Ring_Buffer.Acutator ==  ACTUATOR_SW_RELAY16CH_WRITE)
			{
				if(pPacket[1] == 0x0F && pPacket[5] == 0x10)
				{
					Actuator_SW_Control.State = ACTUATOR_SEND_OK;
				}
				else
				{
					Actuator_SW_Control.State = ACTUATOR_SEND_ERROR;
				}
			}
		}
	}
}

//============================================================================================
void Actuator_OCS_Relay_Control_Proecess(void)
{

#if ACTUATOR_DEBUG
	static uint8_t nTempStep = 0xFF;

    if(nTempStep != Actuator_OCS_Control.Step)
    {
        nTempStep = Actuator_OCS_Control.Step;

        Debug_printf("\nStep = %d", nTempStep);
    }
#endif

	switch(Actuator_OCS_Control.Step)
	{
		case OCS_RELAY_STEP_START :           		// 0
			Actuator_OCS_Next_Step(OCS_RELAY_STEP_WRITE_CMD);
			break;
//===================================================================
		case OCS_RELAY_STEP_WRITE_CMD :				// 1
			Actuator_OCS_Control.RelayState = RELAY_BUSY;
			Actuator_OCS_Control.nRetryCount = 0;

			Actuator_OCS_Next_Step(OCS_RELAY_STEP_STATUS_READ);
			break;
		case OCS_RELAY_STEP_STATUS_READ :			// 2
			if(Actuator.OCS_Relay16ch) Actuator_OCS_Relay_Get_Tx_Data();

			Actuator_OCS_Next_Step(OCS_RELAY_STEP_STATUS_READ_WAIT);
			break;
		case OCS_RELAY_STEP_STATUS_READ_WAIT :		// 3
			if(Actuator_OCS_Control.State == ACTUATOR_RECV_OK) {
				if(Actuator_OCS_Control.nWaitTime >= ACT_SCAN_DELAY) {
					Debug_printf(" OCS nReadRelay : %02X",Actuator_OCS_Control.nReadRelay);

					Actuator_OCS_Next_Step(OCS_RELAY_STEP_WRITE_DATA_MASK);
				}
			}
			else if(Actuator_OCS_Control.State == ACTUATOR_RECV_ERROR || Actuator_OCS_Control.nWaitTime >= ACT_SKIP_DELAY) {
				Actuator_OCS_Next_Step(Actuator_OCS_Control.Step-1);
				Actuator_OCS_Control.nRetryCount++;

				if(Actuator_OCS_Control.nRetryCount >= ACT_RETRY_CNT) {
					Actuator_OCS_Next_Step(OCS_RELAY_STEP_IDLE);
				}
			}
			break;
		case OCS_RELAY_STEP_WRITE_DATA_MASK :		// 4
			switch (OCS_Control[OCS_NUM1].OCSCmd) {
				case OCS_STOP :
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 0);
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 1);
					break;
				case OCS_OPEN :
					BITisSET(Actuator_OCS_Control.nReadRelay, 0);
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 1);
					break;
				case OCS_CLOSE :
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 0);
					BITisSET(Actuator_OCS_Control.nReadRelay, 1);
					break;
				case OCS_TIMED_OPEN :
					BITisSET(Actuator_OCS_Control.nReadRelay, 0);
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 1);
					break;
				case OCS_TIMED_CLOSE :
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 0);
					BITisSET(Actuator_OCS_Control.nReadRelay, 1);
					break;
				case OCS_RUNNING :
					break;
			}
			switch (OCS_Control[OCS_NUM2].OCSCmd) {
				case OCS_STOP :
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 2);
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 3);
					break;
				case OCS_OPEN :
					BITisSET(Actuator_OCS_Control.nReadRelay, 2);
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 3);
					break;
				case OCS_CLOSE :
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 2);
					BITisSET(Actuator_OCS_Control.nReadRelay, 3);
					break;
				case OCS_TIMED_OPEN :
					BITisSET(Actuator_OCS_Control.nReadRelay, 2);
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 3);
					break;
				case OCS_TIMED_CLOSE :
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 2);
					BITisSET(Actuator_OCS_Control.nReadRelay, 3);
					break;
				case OCS_RUNNING :
					break;
			}
			switch (OCS_Control[OCS_NUM3].OCSCmd) {
				case OCS_STOP :
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 4);
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 5);
					break;
				case OCS_OPEN :
					BITisSET(Actuator_OCS_Control.nReadRelay, 4);
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 5);
					break;
				case OCS_CLOSE :
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 4);
					BITisSET(Actuator_OCS_Control.nReadRelay, 5);
					break;
				case OCS_TIMED_OPEN :
					BITisSET(Actuator_OCS_Control.nReadRelay, 4);
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 5);
					break;
				case OCS_TIMED_CLOSE :
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 4);
					BITisSET(Actuator_OCS_Control.nReadRelay, 5);
					break;
				case OCS_RUNNING :
					break;
			}
			switch (OCS_Control[OCS_NUM4].OCSCmd) {
				case OCS_STOP :
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 6);
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 7);
					break;
				case OCS_OPEN :
					BITisSET(Actuator_OCS_Control.nReadRelay, 6);
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 7);
					break;
				case OCS_CLOSE :
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 6);
					BITisSET(Actuator_OCS_Control.nReadRelay, 7);
					break;
				case OCS_TIMED_OPEN :
					BITisSET(Actuator_OCS_Control.nReadRelay, 6);
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 7);
					break;
				case OCS_TIMED_CLOSE :
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 6);
					BITisSET(Actuator_OCS_Control.nReadRelay, 7);
					break;
				case OCS_RUNNING :
					break;
			}
			switch (OCS_Control[OCS_NUM5].OCSCmd) {
				case OCS_STOP :
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 8);
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 9);
					break;
				case OCS_OPEN :
					BITisSET(Actuator_OCS_Control.nReadRelay, 8);
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 9);
					break;
				case OCS_CLOSE :
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 8);
					BITisSET(Actuator_OCS_Control.nReadRelay, 9);
					break;
				case OCS_TIMED_OPEN :
					BITisSET(Actuator_OCS_Control.nReadRelay, 8);
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 9);
					break;
				case OCS_TIMED_CLOSE :
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 8);
					BITisSET(Actuator_OCS_Control.nReadRelay, 9);
					break;
				case OCS_RUNNING :
					break;
			}
			switch (OCS_Control[OCS_NUM6].OCSCmd) {
				case OCS_STOP :
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 10);
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 11);
					break;
				case OCS_OPEN :
					BITisSET(Actuator_OCS_Control.nReadRelay, 10);
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 11);
					break;
				case OCS_CLOSE :
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 10);
					BITisSET(Actuator_OCS_Control.nReadRelay, 11);
					break;
				case OCS_TIMED_OPEN :
					BITisSET(Actuator_OCS_Control.nReadRelay, 10);
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 11);
					break;
				case OCS_TIMED_CLOSE :
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 10);
					BITisSET(Actuator_OCS_Control.nReadRelay, 11);
					break;
				case OCS_RUNNING :
					break;
			}
			switch (OCS_Control[OCS_NUM7].OCSCmd) {
				case OCS_STOP :
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 12);
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 13);
					break;
				case OCS_OPEN :
					BITisSET(Actuator_OCS_Control.nReadRelay, 12);
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 13);
					break;
				case OCS_CLOSE :
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 12);
					BITisSET(Actuator_OCS_Control.nReadRelay, 13);
					break;
				case OCS_TIMED_OPEN :
					BITisSET(Actuator_OCS_Control.nReadRelay, 12);
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 13);
					break;
				case OCS_TIMED_CLOSE :
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 12);
					BITisSET(Actuator_OCS_Control.nReadRelay, 13);
					break;
				case OCS_RUNNING :
					break;
			}
			switch (OCS_Control[OCS_NUM8].OCSCmd) {
				case OCS_STOP :
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 14);
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 15);
					break;
				case OCS_OPEN :
					BITisSET(Actuator_OCS_Control.nReadRelay, 14);
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 15);
					break;
				case OCS_CLOSE :
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 14);
					BITisSET(Actuator_OCS_Control.nReadRelay, 15);
					break;
				case OCS_TIMED_OPEN :
					BITisSET(Actuator_OCS_Control.nReadRelay, 14);
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 15);
					break;
				case OCS_TIMED_CLOSE :
					BITisCLEAR(Actuator_OCS_Control.nReadRelay, 14);
					BITisSET(Actuator_OCS_Control.nReadRelay, 15);
					break;
				case OCS_RUNNING :
					break;
			}

			if(NodeVersion == RELAY_32CH_VER)
			{
				switch (OCS_Control[OCS_NUM9].OCSCmd)
				{
					case OCS_STOP :
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 16);
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 17);
						break;
					case OCS_OPEN :
						BITisSET(Actuator_OCS_Control.nReadRelay, 16);
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 17);
						break;
					case OCS_CLOSE :
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 16);
						BITisSET(Actuator_OCS_Control.nReadRelay, 17);
						break;
					case OCS_TIMED_OPEN :
						BITisSET(Actuator_OCS_Control.nReadRelay, 16);
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 17);
						break;
					case OCS_TIMED_CLOSE :
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 16);
						BITisSET(Actuator_OCS_Control.nReadRelay, 17);
						break;
					case OCS_RUNNING :
						break;
				}
				switch (OCS_Control[OCS_NUM10].OCSCmd)
				{
					case OCS_STOP :
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 18);
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 19);
						break;
					case OCS_OPEN :
						BITisSET(Actuator_OCS_Control.nReadRelay, 18);
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 19);
						break;
					case OCS_CLOSE :
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 18);
						BITisSET(Actuator_OCS_Control.nReadRelay, 19);
						break;
					case OCS_TIMED_OPEN :
						BITisSET(Actuator_OCS_Control.nReadRelay, 18);
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 19);
						break;
					case OCS_TIMED_CLOSE :
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 18);
						BITisSET(Actuator_OCS_Control.nReadRelay, 19);
						break;
					case OCS_RUNNING :
						break;
				}
				switch (OCS_Control[OCS_NUM11].OCSCmd)
				{
					case OCS_STOP :
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 20);
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 21);
						break;
					case OCS_OPEN :
						BITisSET(Actuator_OCS_Control.nReadRelay, 20);
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 21);
						break;
					case OCS_CLOSE :
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 20);
						BITisSET(Actuator_OCS_Control.nReadRelay, 21);
						break;
					case OCS_TIMED_OPEN :
						BITisSET(Actuator_OCS_Control.nReadRelay, 20);
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 21);
						break;
					case OCS_TIMED_CLOSE :
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 20);
						BITisSET(Actuator_OCS_Control.nReadRelay, 21);
						break;
					case OCS_RUNNING :
						break;
				}
				switch (OCS_Control[OCS_NUM12].OCSCmd)
				{
					case OCS_STOP :
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 22);
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 23);
						break;
					case OCS_OPEN :
						BITisSET(Actuator_OCS_Control.nReadRelay, 22);
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 23);
						break;
					case OCS_CLOSE :
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 22);
						BITisSET(Actuator_OCS_Control.nReadRelay, 23);
						break;
					case OCS_TIMED_OPEN :
						BITisSET(Actuator_OCS_Control.nReadRelay, 22);
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 23);
						break;
					case OCS_TIMED_CLOSE :
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 22);
						BITisSET(Actuator_OCS_Control.nReadRelay, 23);
						break;
					case OCS_RUNNING :
						break;
				}
				switch (OCS_Control[OCS_NUM13].OCSCmd)
				{
					case OCS_STOP :
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 24);
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 25);
						break;
					case OCS_OPEN :
						BITisSET(Actuator_OCS_Control.nReadRelay, 24);
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 25);
						break;
					case OCS_CLOSE :
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 24);
						BITisSET(Actuator_OCS_Control.nReadRelay, 25);
						break;
					case OCS_TIMED_OPEN :
						BITisSET(Actuator_OCS_Control.nReadRelay, 24);
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 25);
						break;
					case OCS_TIMED_CLOSE :
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 24);
						BITisSET(Actuator_OCS_Control.nReadRelay, 25);
						break;
					case OCS_RUNNING :
						break;
				}
				switch (OCS_Control[OCS_NUM14].OCSCmd)
				{
					case OCS_STOP :
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 26);
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 27);
						break;
					case OCS_OPEN :
						BITisSET(Actuator_OCS_Control.nReadRelay, 26);
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 27);
						break;
					case OCS_CLOSE :
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 26);
						BITisSET(Actuator_OCS_Control.nReadRelay, 27);
						break;
					case OCS_TIMED_OPEN :
						BITisSET(Actuator_OCS_Control.nReadRelay, 26);
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 27);
						break;
					case OCS_TIMED_CLOSE :
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 26);
						BITisSET(Actuator_OCS_Control.nReadRelay, 27);
						break;
					case OCS_RUNNING :
						break;
				}
				switch (OCS_Control[OCS_NUM15].OCSCmd)
				{
					case OCS_STOP :
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 28);
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 29);
						break;
					case OCS_OPEN :
						BITisSET(Actuator_OCS_Control.nReadRelay, 28);
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 29);
						break;
					case OCS_CLOSE :
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 28);
						BITisSET(Actuator_OCS_Control.nReadRelay, 29);
						break;
					case OCS_TIMED_OPEN :
						BITisSET(Actuator_OCS_Control.nReadRelay, 28);
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 29);
						break;
					case OCS_TIMED_CLOSE :
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 28);
						BITisSET(Actuator_OCS_Control.nReadRelay, 29);
						break;
					case OCS_RUNNING :
						break;
				}
				switch (OCS_Control[OCS_NUM16].OCSCmd)
				{
					case OCS_STOP :
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 30);
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 31);
						break;
					case OCS_OPEN :
						BITisSET(Actuator_OCS_Control.nReadRelay, 30);
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 31);
						break;
					case OCS_CLOSE :
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 30);
						BITisSET(Actuator_OCS_Control.nReadRelay, 31);
						break;
					case OCS_TIMED_OPEN :
						BITisSET(Actuator_OCS_Control.nReadRelay, 30);
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 31);
						break;
					case OCS_TIMED_CLOSE :
						BITisCLEAR(Actuator_OCS_Control.nReadRelay, 30);
						BITisSET(Actuator_OCS_Control.nReadRelay, 31);
						break;
					case OCS_RUNNING :
						break;
				}
			}

			Actuator_OCS_Control.nWriteRelay = Actuator_OCS_Control.nReadRelay;
			Actuator_OCS_Next_Step(OCS_RELAY_STEP_DATA_WRITE);
			break;
		case OCS_RELAY_STEP_DATA_WRITE :			// 5
			Debug_printf(" OCS nWriteRelay : %02X\n",Actuator_OCS_Control.nWriteRelay);
			Actuator_OCS_Relay_Set_Tx_Data(Actuator_OCS_Control.nWriteRelay);

			Actuator_OCS_Next_Step(OCS_RELAY_STEP_DATA_WRITE_WAIT);
			break;
		case OCS_RELAY_STEP_DATA_WRITE_WAIT :		// 6
			if(Actuator_OCS_Control.State == ACTUATOR_SEND_OK) {
				switch (OCS_Control[OCS_NUM1].OCSCmd) {
					case OCS_STOP :
						OCS_Control[OCS_NUM1].OCStatus = OCS_RUN_STOP;
						break;
					case OCS_OPEN :
						OCS_Control[OCS_NUM1].OCStatus = OCS_OPEN_START;
						OCS_Control[OCS_NUM1].OCSCmd = OCS_RUNNING;
						break;
					case OCS_CLOSE :
						OCS_Control[OCS_NUM1].OCStatus = OCS_CLOSE_START;
						OCS_Control[OCS_NUM1].OCSCmd = OCS_RUNNING;
						break;
					case OCS_TIMED_OPEN :
						OCS_Control[OCS_NUM1].OCStatus = OCS_TIME_OPEN_START;
						OCS_Control[OCS_NUM1].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM1].mTMR_1Sec);
						break;
					case OCS_TIMED_CLOSE :
						OCS_Control[OCS_NUM1].OCStatus = OCS_TIME_CLOSE_START;
						OCS_Control[OCS_NUM1].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM1].mTMR_1Sec);
						break;
					case OCS_RUNNING :
						break;
				}
				switch (OCS_Control[OCS_NUM2].OCSCmd) {
					case OCS_STOP :
						OCS_Control[OCS_NUM2].OCStatus = OCS_RUN_STOP;
						break;
					case OCS_OPEN :
						OCS_Control[OCS_NUM2].OCStatus = OCS_OPEN_START;
						OCS_Control[OCS_NUM2].OCSCmd = OCS_RUNNING;
						break;
					case OCS_CLOSE :
						OCS_Control[OCS_NUM2].OCStatus = OCS_CLOSE_START;
						OCS_Control[OCS_NUM2].OCSCmd = OCS_RUNNING;
						break;
					case OCS_TIMED_OPEN :
						OCS_Control[OCS_NUM2].OCStatus = OCS_TIME_OPEN_START;
						OCS_Control[OCS_NUM2].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM2].mTMR_1Sec);
						break;
					case OCS_TIMED_CLOSE :
						OCS_Control[OCS_NUM2].OCStatus = OCS_TIME_CLOSE_START;
						OCS_Control[OCS_NUM2].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM2].mTMR_1Sec);
						break;
					case OCS_RUNNING :
						break;
				}
				switch (OCS_Control[OCS_NUM3].OCSCmd) {
					case OCS_STOP :
						OCS_Control[OCS_NUM3].OCStatus = OCS_RUN_STOP;
						break;
					case OCS_OPEN :
						OCS_Control[OCS_NUM3].OCStatus = OCS_OPEN_START;
						OCS_Control[OCS_NUM3].OCSCmd = OCS_RUNNING;
						break;
					case OCS_CLOSE :
						OCS_Control[OCS_NUM3].OCStatus = OCS_CLOSE_START;
						OCS_Control[OCS_NUM3].OCSCmd = OCS_RUNNING;
						break;
					case OCS_TIMED_OPEN :
						OCS_Control[OCS_NUM3].OCStatus = OCS_TIME_OPEN_START;
						OCS_Control[OCS_NUM3].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM3].mTMR_1Sec);
						break;
					case OCS_TIMED_CLOSE :
						OCS_Control[OCS_NUM3].OCStatus = OCS_TIME_CLOSE_START;
						OCS_Control[OCS_NUM3].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM3].mTMR_1Sec);
						break;
					case OCS_RUNNING :
						break;
				}
				switch (OCS_Control[OCS_NUM4].OCSCmd) {
					case OCS_STOP :
						OCS_Control[OCS_NUM4].OCStatus = OCS_RUN_STOP;
						break;
					case OCS_OPEN :
						OCS_Control[OCS_NUM4].OCStatus = OCS_OPEN_START;
						OCS_Control[OCS_NUM4].OCSCmd = OCS_RUNNING;
						break;
					case OCS_CLOSE :
						OCS_Control[OCS_NUM4].OCStatus = OCS_CLOSE_START;
						OCS_Control[OCS_NUM4].OCSCmd = OCS_RUNNING;
						break;
					case OCS_TIMED_OPEN :
						OCS_Control[OCS_NUM4].OCStatus = OCS_TIME_OPEN_START;
						OCS_Control[OCS_NUM4].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM4].mTMR_1Sec);
						break;
					case OCS_TIMED_CLOSE :
						OCS_Control[OCS_NUM4].OCStatus = OCS_TIME_CLOSE_START;
						OCS_Control[OCS_NUM4].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM4].mTMR_1Sec);
						break;
					case OCS_RUNNING :
						break;
				}
				switch (OCS_Control[OCS_NUM5].OCSCmd) {
					case OCS_STOP :
						OCS_Control[OCS_NUM5].OCStatus = OCS_RUN_STOP;
						break;
					case OCS_OPEN :
						OCS_Control[OCS_NUM5].OCStatus = OCS_OPEN_START;
						OCS_Control[OCS_NUM5].OCSCmd = OCS_RUNNING;
						break;
					case OCS_CLOSE :
						OCS_Control[OCS_NUM5].OCStatus = OCS_CLOSE_START;
						OCS_Control[OCS_NUM5].OCSCmd = OCS_RUNNING;
						break;
					case OCS_TIMED_OPEN :
						OCS_Control[OCS_NUM5].OCStatus = OCS_TIME_OPEN_START;
						OCS_Control[OCS_NUM5].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM5].mTMR_1Sec);
						break;
					case OCS_TIMED_CLOSE :
						OCS_Control[OCS_NUM5].OCStatus = OCS_TIME_CLOSE_START;
						OCS_Control[OCS_NUM5].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM5].mTMR_1Sec);
						break;
					case OCS_RUNNING :
						break;
				}
				switch (OCS_Control[OCS_NUM6].OCSCmd) {
					case OCS_STOP :
						OCS_Control[OCS_NUM6].OCStatus = OCS_RUN_STOP;
						break;
					case OCS_OPEN :
						OCS_Control[OCS_NUM6].OCStatus = OCS_OPEN_START;
						OCS_Control[OCS_NUM6].OCSCmd = OCS_RUNNING;
						break;
					case OCS_CLOSE :
						OCS_Control[OCS_NUM6].OCStatus = OCS_CLOSE_START;
						OCS_Control[OCS_NUM6].OCSCmd = OCS_RUNNING;
						break;
					case OCS_TIMED_OPEN :
						OCS_Control[OCS_NUM6].OCStatus = OCS_TIME_OPEN_START;
						OCS_Control[OCS_NUM6].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM6].mTMR_1Sec);
						break;
					case OCS_TIMED_CLOSE :
						OCS_Control[OCS_NUM6].OCStatus = OCS_TIME_CLOSE_START;
						OCS_Control[OCS_NUM6].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM6].mTMR_1Sec);
						break;
					case OCS_RUNNING :
						break;
				}
				switch (OCS_Control[OCS_NUM7].OCSCmd) {
					case OCS_STOP :
						OCS_Control[OCS_NUM7].OCStatus = OCS_RUN_STOP;
						break;
					case OCS_OPEN :
						OCS_Control[OCS_NUM7].OCStatus = OCS_OPEN_START;
						OCS_Control[OCS_NUM7].OCSCmd = OCS_RUNNING;
						break;
					case OCS_CLOSE :
						OCS_Control[OCS_NUM7].OCStatus = OCS_CLOSE_START;
						OCS_Control[OCS_NUM7].OCSCmd = OCS_RUNNING;
						break;
					case OCS_TIMED_OPEN :
						OCS_Control[OCS_NUM7].OCStatus = OCS_TIME_OPEN_START;
						OCS_Control[OCS_NUM7].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM7].mTMR_1Sec);
						break;
					case OCS_TIMED_CLOSE :
						OCS_Control[OCS_NUM7].OCStatus = OCS_TIME_CLOSE_START;
						OCS_Control[OCS_NUM7].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM7].mTMR_1Sec);
						break;
					case OCS_RUNNING :
						break;
				}
				switch (OCS_Control[OCS_NUM8].OCSCmd) {
					case OCS_STOP :
						OCS_Control[OCS_NUM8].OCStatus = OCS_RUN_STOP;
						break;
					case OCS_OPEN :
						OCS_Control[OCS_NUM8].OCStatus = OCS_OPEN_START;
						OCS_Control[OCS_NUM8].OCSCmd = OCS_RUNNING;
						break;
					case OCS_CLOSE :
						OCS_Control[OCS_NUM8].OCStatus = OCS_CLOSE_START;
						OCS_Control[OCS_NUM8].OCSCmd = OCS_RUNNING;
						break;
					case OCS_TIMED_OPEN :
						OCS_Control[OCS_NUM8].OCStatus = OCS_TIME_OPEN_START;
						OCS_Control[OCS_NUM8].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM8].mTMR_1Sec);
						break;
					case OCS_TIMED_CLOSE :
						OCS_Control[OCS_NUM8].OCStatus = OCS_TIME_CLOSE_START;
						OCS_Control[OCS_NUM8].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM8].mTMR_1Sec);
						break;
					case OCS_RUNNING :
						break;
				}

				if(NodeVersion == RELAY_32CH_VER)
				{
					switch (OCS_Control[OCS_NUM9].OCSCmd)
					{
						case OCS_STOP :
							OCS_Control[OCS_NUM9].OCStatus = OCS_RUN_STOP;
							break;
						case OCS_OPEN :
							OCS_Control[OCS_NUM9].OCStatus = OCS_OPEN_START;
							OCS_Control[OCS_NUM9].OCSCmd = OCS_RUNNING;
							break;
						case OCS_CLOSE :
							OCS_Control[OCS_NUM9].OCStatus = OCS_CLOSE_START;
							OCS_Control[OCS_NUM9].OCSCmd = OCS_RUNNING;
							break;
						case OCS_TIMED_OPEN :
							OCS_Control[OCS_NUM9].OCStatus = OCS_TIME_OPEN_START;
							OCS_Control[OCS_NUM9].OCSCmd = OCS_RUNNING;
							sec_timer_reset(OCS_Control[OCS_NUM9].mTMR_1Sec);
							break;
						case OCS_TIMED_CLOSE :
							OCS_Control[OCS_NUM9].OCStatus = OCS_TIME_CLOSE_START;
							OCS_Control[OCS_NUM9].OCSCmd = OCS_RUNNING;
							sec_timer_reset(OCS_Control[OCS_NUM9].mTMR_1Sec);
							break;
						case OCS_RUNNING :
							break;
					}
					switch (OCS_Control[OCS_NUM10].OCSCmd)
					{
						case OCS_STOP :
							OCS_Control[OCS_NUM10].OCStatus = OCS_RUN_STOP;
							break;
						case OCS_OPEN :
							OCS_Control[OCS_NUM10].OCStatus = OCS_OPEN_START;
							OCS_Control[OCS_NUM10].OCSCmd = OCS_RUNNING;
							break;
						case OCS_CLOSE :
							OCS_Control[OCS_NUM10].OCStatus = OCS_CLOSE_START;
							OCS_Control[OCS_NUM10].OCSCmd = OCS_RUNNING;
							break;
						case OCS_TIMED_OPEN :
							OCS_Control[OCS_NUM10].OCStatus = OCS_TIME_OPEN_START;
							OCS_Control[OCS_NUM10].OCSCmd = OCS_RUNNING;
							sec_timer_reset(OCS_Control[OCS_NUM10].mTMR_1Sec);
							break;
						case OCS_TIMED_CLOSE :
							OCS_Control[OCS_NUM10].OCStatus = OCS_TIME_CLOSE_START;
							OCS_Control[OCS_NUM10].OCSCmd = OCS_RUNNING;
							sec_timer_reset(OCS_Control[OCS_NUM10].mTMR_1Sec);
							break;
						case OCS_RUNNING :
							break;
					}
					switch (OCS_Control[OCS_NUM11].OCSCmd)
					{
						case OCS_STOP :
							OCS_Control[OCS_NUM11].OCStatus = OCS_RUN_STOP;
							break;
						case OCS_OPEN :
							OCS_Control[OCS_NUM11].OCStatus = OCS_OPEN_START;
							OCS_Control[OCS_NUM11].OCSCmd = OCS_RUNNING;
							break;
						case OCS_CLOSE :
							OCS_Control[OCS_NUM11].OCStatus = OCS_CLOSE_START;
							OCS_Control[OCS_NUM11].OCSCmd = OCS_RUNNING;
							break;
						case OCS_TIMED_OPEN :
							OCS_Control[OCS_NUM11].OCStatus = OCS_TIME_OPEN_START;
							OCS_Control[OCS_NUM11].OCSCmd = OCS_RUNNING;
							sec_timer_reset(OCS_Control[OCS_NUM11].mTMR_1Sec);
							break;
						case OCS_TIMED_CLOSE :
							OCS_Control[OCS_NUM11].OCStatus = OCS_TIME_CLOSE_START;
							OCS_Control[OCS_NUM11].OCSCmd = OCS_RUNNING;
							sec_timer_reset(OCS_Control[OCS_NUM11].mTMR_1Sec);
							break;
						case OCS_RUNNING :
							break;
					}
					switch (OCS_Control[OCS_NUM12].OCSCmd)
					{
						case OCS_STOP :
							OCS_Control[OCS_NUM12].OCStatus = OCS_RUN_STOP;
							break;
						case OCS_OPEN :
							OCS_Control[OCS_NUM12].OCStatus = OCS_OPEN_START;
							OCS_Control[OCS_NUM12].OCSCmd = OCS_RUNNING;
							break;
						case OCS_CLOSE :
							OCS_Control[OCS_NUM12].OCStatus = OCS_CLOSE_START;
							OCS_Control[OCS_NUM12].OCSCmd = OCS_RUNNING;
							break;
						case OCS_TIMED_OPEN :
							OCS_Control[OCS_NUM12].OCStatus = OCS_TIME_OPEN_START;
							OCS_Control[OCS_NUM12].OCSCmd = OCS_RUNNING;
							sec_timer_reset(OCS_Control[OCS_NUM12].mTMR_1Sec);
							break;
						case OCS_TIMED_CLOSE :
							OCS_Control[OCS_NUM12].OCStatus = OCS_TIME_CLOSE_START;
							OCS_Control[OCS_NUM12].OCSCmd = OCS_RUNNING;
							sec_timer_reset(OCS_Control[OCS_NUM12].mTMR_1Sec);
							break;
						case OCS_RUNNING :
							break;
					}
					switch (OCS_Control[OCS_NUM13].OCSCmd)
					{
						case OCS_STOP :
							OCS_Control[OCS_NUM13].OCStatus = OCS_RUN_STOP;
							break;
						case OCS_OPEN :
							OCS_Control[OCS_NUM13].OCStatus = OCS_OPEN_START;
							OCS_Control[OCS_NUM13].OCSCmd = OCS_RUNNING;
							break;
						case OCS_CLOSE :
							OCS_Control[OCS_NUM13].OCStatus = OCS_CLOSE_START;
							OCS_Control[OCS_NUM13].OCSCmd = OCS_RUNNING;
							break;
						case OCS_TIMED_OPEN :
							OCS_Control[OCS_NUM13].OCStatus = OCS_TIME_OPEN_START;
							OCS_Control[OCS_NUM13].OCSCmd = OCS_RUNNING;
							sec_timer_reset(OCS_Control[OCS_NUM13].mTMR_1Sec);
							break;
						case OCS_TIMED_CLOSE :
							OCS_Control[OCS_NUM13].OCStatus = OCS_TIME_CLOSE_START;
							OCS_Control[OCS_NUM13].OCSCmd = OCS_RUNNING;
							sec_timer_reset(OCS_Control[OCS_NUM13].mTMR_1Sec);
							break;
						case OCS_RUNNING :
							break;
					}
					switch (OCS_Control[OCS_NUM14].OCSCmd)
					{
						case OCS_STOP :
							OCS_Control[OCS_NUM14].OCStatus = OCS_RUN_STOP;
							break;
						case OCS_OPEN :
							OCS_Control[OCS_NUM14].OCStatus = OCS_OPEN_START;
							OCS_Control[OCS_NUM14].OCSCmd = OCS_RUNNING;
							break;
						case OCS_CLOSE :
							OCS_Control[OCS_NUM14].OCStatus = OCS_CLOSE_START;
							OCS_Control[OCS_NUM14].OCSCmd = OCS_RUNNING;
							break;
						case OCS_TIMED_OPEN :
							OCS_Control[OCS_NUM14].OCStatus = OCS_TIME_OPEN_START;
							OCS_Control[OCS_NUM14].OCSCmd = OCS_RUNNING;
							sec_timer_reset(OCS_Control[OCS_NUM14].mTMR_1Sec);
							break;
						case OCS_TIMED_CLOSE :
							OCS_Control[OCS_NUM14].OCStatus = OCS_TIME_CLOSE_START;
							OCS_Control[OCS_NUM14].OCSCmd = OCS_RUNNING;
							sec_timer_reset(OCS_Control[OCS_NUM14].mTMR_1Sec);
							break;
						case OCS_RUNNING :
							break;
					}
					switch (OCS_Control[OCS_NUM15].OCSCmd)
					{
						case OCS_STOP :
							OCS_Control[OCS_NUM15].OCStatus = OCS_RUN_STOP;
							break;
						case OCS_OPEN :
							OCS_Control[OCS_NUM15].OCStatus = OCS_OPEN_START;
							OCS_Control[OCS_NUM15].OCSCmd = OCS_RUNNING;
							break;
						case OCS_CLOSE :
							OCS_Control[OCS_NUM15].OCStatus = OCS_CLOSE_START;
							OCS_Control[OCS_NUM15].OCSCmd = OCS_RUNNING;
							break;
						case OCS_TIMED_OPEN :
							OCS_Control[OCS_NUM15].OCStatus = OCS_TIME_OPEN_START;
							OCS_Control[OCS_NUM15].OCSCmd = OCS_RUNNING;
							sec_timer_reset(OCS_Control[OCS_NUM15].mTMR_1Sec);
							break;
						case OCS_TIMED_CLOSE :
							OCS_Control[OCS_NUM15].OCStatus = OCS_TIME_CLOSE_START;
							OCS_Control[OCS_NUM15].OCSCmd = OCS_RUNNING;
							sec_timer_reset(OCS_Control[OCS_NUM15].mTMR_1Sec);
							break;
						case OCS_RUNNING :
							break;
					}
					switch (OCS_Control[OCS_NUM16].OCSCmd)
					{
						case OCS_STOP :
							OCS_Control[OCS_NUM16].OCStatus = OCS_RUN_STOP;
							break;
						case OCS_OPEN :
							OCS_Control[OCS_NUM16].OCStatus = OCS_OPEN_START;
							OCS_Control[OCS_NUM16].OCSCmd = OCS_RUNNING;
							break;
						case OCS_CLOSE :
							OCS_Control[OCS_NUM16].OCStatus = OCS_CLOSE_START;
							OCS_Control[OCS_NUM16].OCSCmd = OCS_RUNNING;
							break;
						case OCS_TIMED_OPEN :
							OCS_Control[OCS_NUM16].OCStatus = OCS_TIME_OPEN_START;
							OCS_Control[OCS_NUM16].OCSCmd = OCS_RUNNING;
							sec_timer_reset(OCS_Control[OCS_NUM16].mTMR_1Sec);
							break;
						case OCS_TIMED_CLOSE :
							OCS_Control[OCS_NUM16].OCStatus = OCS_TIME_CLOSE_START;
							OCS_Control[OCS_NUM16].OCSCmd = OCS_RUNNING;
							sec_timer_reset(OCS_Control[OCS_NUM16].mTMR_1Sec);
							break;
						case OCS_RUNNING :
							break;
					}
				}

				Actuator_OCS_Next_Step(OCS_RELAY_STEP_IDLE);
			}
			else if(Actuator_OCS_Control.State == ACTUATOR_SEND_ERROR || Actuator_OCS_Control.nWaitTime >= ACT_SKIP_DELAY) {
				Actuator_OCS_Next_Step(Actuator_OCS_Control.Step - 1);
				Actuator_OCS_Control.nRetryCount++;

				if(Actuator_OCS_Control.nRetryCount >= ACT_RETRY_CNT) {
					Actuator_OCS_Next_Step(OCS_RELAY_STEP_IDLE);
				}
			}
			break;

		case OCS_RELAY_STEP_IDLE :					// 7
			Actuator_OCS_Control.RelayState = RELAY_READY;
			break;
	}
}

void Actuator_OCS_Next_Step(OCS_RELAY_STEP step)
{
	Actuator_OCS_Control.Step = step;
	Actuator_OCS_Control.nWaitTime = 0;
	Actuator_OCS_Control.State = ACTUATOR_UNKNONE;

	Actuator_Ring_Buffer.nProcessIndex = Actuator_Ring_Buffer.nSaveIndex = 0;
}

//============================================================================================
void Actuator_OCS_EXT_Relay_Control_Proecess(void)
{

#if ACTUATOR_DEBUG
	static uint8_t nTempStep = 0xFF;

    if(nTempStep != Actuator_OCS_EXT_Control.Step)
    {
        nTempStep = Actuator_OCS_EXT_Control.Step;

        Debug_printf("\nStep = %d", nTempStep);
    }
#endif

	switch(Actuator_OCS_EXT_Control.Step)
	{
		case OCS_EXT_RELAY_STEP_START :           		// 0

			Actuator_OCS_EXT_Next_Step(OCS_EXT_RELAY_STEP_WRITE_CMD);
			break;
//===================================================================
		case OCS_EXT_RELAY_STEP_WRITE_CMD :				// 1
			Actuator_OCS_EXT_Control.RelayState = RELAY_BUSY;
			Actuator_OCS_EXT_Control.nRetryCount = 0;

			Actuator_OCS_EXT_Next_Step(OCS_EXT_RELAY_STEP_STATUS_READ);
			break;
		case OCS_EXT_RELAY_STEP_STATUS_READ :			// 2
			if(Actuator.OCS_Relay16ch_EXT) Actuator_OCS_EXT_Relay_Get_Tx_Data();

			Actuator_OCS_EXT_Next_Step(OCS_EXT_RELAY_STEP_STATUS_READ_WAIT);
			break;
		case OCS_EXT_RELAY_STEP_STATUS_READ_WAIT :		// 3
			if(Actuator_OCS_EXT_Control.State == ACTUATOR_RECV_OK)
			{
				if(Actuator_OCS_EXT_Control.nWaitTime >= ACT_SCAN_DELAY)
				{
					Debug_printf(" OCS EXT nReadRelay : %02X",Actuator_OCS_EXT_Control.nReadRelay);

					Actuator_OCS_EXT_Next_Step(OCS_EXT_RELAY_STEP_WRITE_DATA_MASK);
				}
			}
			else if(Actuator_OCS_EXT_Control.State == ACTUATOR_RECV_ERROR || Actuator_OCS_EXT_Control.nWaitTime >= ACT_SKIP_DELAY)
			{
				Actuator_OCS_EXT_Next_Step(Actuator_OCS_EXT_Control.Step-1);
				Actuator_OCS_EXT_Control.nRetryCount++;

				if(Actuator_OCS_EXT_Control.nRetryCount >= ACT_RETRY_CNT)
				{
					Actuator_OCS_EXT_Next_Step(OCS_EXT_RELAY_STEP_IDLE);
				}
			}
			break;
		case OCS_EXT_RELAY_STEP_WRITE_DATA_MASK :		// 4
			switch (OCS_Control[OCS_NUM9].OCSCmd)
			{
				case OCS_STOP :
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 0);
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 1);
					break;
				case OCS_OPEN :
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 0);
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 1);
					break;
				case OCS_CLOSE :
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 0);
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 1);
					break;
				case OCS_TIMED_OPEN :
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 0);
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 1);
					break;
				case OCS_TIMED_CLOSE :
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 0);
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 1);
					break;
				case OCS_RUNNING :
					break;
			}
			switch (OCS_Control[OCS_NUM10].OCSCmd)
			{
				case OCS_STOP :
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 2);
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 3);
					break;
				case OCS_OPEN :
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 2);
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 3);
					break;
				case OCS_CLOSE :
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 2);
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 3);
					break;
				case OCS_TIMED_OPEN :
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 2);
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 3);
					break;
				case OCS_TIMED_CLOSE :
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 2);
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 3);
					break;
				case OCS_RUNNING :
					break;
			}
			switch (OCS_Control[OCS_NUM11].OCSCmd)
			{
				case OCS_STOP :
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 4);
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 5);
					break;
				case OCS_OPEN :
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 4);
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 5);
					break;
				case OCS_CLOSE :
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 4);
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 5);
					break;
				case OCS_TIMED_OPEN :
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 4);
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 5);
					break;
				case OCS_TIMED_CLOSE :
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 4);
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 5);
					break;
				case OCS_RUNNING :
					break;
			}
			switch (OCS_Control[OCS_NUM12].OCSCmd)
			{
				case OCS_STOP :
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 6);
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 7);
					break;
				case OCS_OPEN :
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 6);
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 7);
					break;
				case OCS_CLOSE :
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 6);
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 7);
					break;
				case OCS_TIMED_OPEN :
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 6);
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 7);
					break;
				case OCS_TIMED_CLOSE :
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 6);
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 7);
					break;
				case OCS_RUNNING :
					break;
			}
			switch (OCS_Control[OCS_NUM13].OCSCmd)
			{
				case OCS_STOP :
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 8);
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 9);
					break;
				case OCS_OPEN :
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 8);
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 9);
					break;
				case OCS_CLOSE :
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 8);
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 9);
					break;
				case OCS_TIMED_OPEN :
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 8);
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 9);
					break;
				case OCS_TIMED_CLOSE :
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 8);
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 9);
					break;
				case OCS_RUNNING :
					break;
			}
			switch (OCS_Control[OCS_NUM14].OCSCmd)
			{
				case OCS_STOP :
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 10);
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 11);
					break;
				case OCS_OPEN :
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 10);
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 11);
					break;
				case OCS_CLOSE :
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 10);
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 11);
					break;
				case OCS_TIMED_OPEN :
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 10);
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 11);
					break;
				case OCS_TIMED_CLOSE :
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 10);
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 11);
					break;
				case OCS_RUNNING :
					break;
			}
			switch (OCS_Control[OCS_NUM15].OCSCmd)
			{
				case OCS_STOP :
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 12);
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 13);
					break;
				case OCS_OPEN :
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 12);
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 13);
					break;
				case OCS_CLOSE :
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 12);
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 13);
					break;
				case OCS_TIMED_OPEN :
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 12);
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 13);
					break;
				case OCS_TIMED_CLOSE :
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 12);
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 13);
					break;
				case OCS_RUNNING :
					break;
			}
			switch (OCS_Control[OCS_NUM16].OCSCmd)
			{
				case OCS_STOP :
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 14);
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 15);
					break;
				case OCS_OPEN :
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 14);
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 15);
					break;
				case OCS_CLOSE :
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 14);
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 15);
					break;
				case OCS_TIMED_OPEN :
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 14);
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 15);
					break;
				case OCS_TIMED_CLOSE :
					BITisCLEAR(Actuator_OCS_EXT_Control.nReadRelay, 14);
					BITisSET(Actuator_OCS_EXT_Control.nReadRelay, 15);
					break;
				case OCS_RUNNING :
					break;
			}
			Actuator_OCS_EXT_Control.nWriteRelay = Actuator_OCS_EXT_Control.nReadRelay;
			Actuator_OCS_EXT_Next_Step(OCS_EXT_RELAY_STEP_DATA_WRITE);
			break;
		case OCS_EXT_RELAY_STEP_DATA_WRITE :			// 5
			Debug_printf(" OCS EXT nWriteRelay : %02X\n",Actuator_OCS_EXT_Control.nWriteRelay);
			Actuator_OCS_EXT_Relay_Set_Tx_Data(Actuator_OCS_EXT_Control.nWriteRelay);

			Actuator_OCS_EXT_Next_Step(OCS_EXT_RELAY_STEP_DATA_WRITE_WAIT);
			break;
		case OCS_EXT_RELAY_STEP_DATA_WRITE_WAIT :		// 6
			if(Actuator_OCS_EXT_Control.State == ACTUATOR_SEND_OK)
			{
				switch (OCS_Control[OCS_NUM9].OCSCmd)
				{
					case OCS_STOP :
						OCS_Control[OCS_NUM9].OCStatus = OCS_RUN_STOP;
						break;
					case OCS_OPEN :
						OCS_Control[OCS_NUM9].OCStatus = OCS_OPEN_START;
						OCS_Control[OCS_NUM9].OCSCmd = OCS_RUNNING;
						break;
					case OCS_CLOSE :
						OCS_Control[OCS_NUM9].OCStatus = OCS_CLOSE_START;
						OCS_Control[OCS_NUM9].OCSCmd = OCS_RUNNING;
						break;
					case OCS_TIMED_OPEN :
						OCS_Control[OCS_NUM9].OCStatus = OCS_TIME_OPEN_START;
						OCS_Control[OCS_NUM9].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM9].mTMR_1Sec);
						break;
					case OCS_TIMED_CLOSE :
						OCS_Control[OCS_NUM9].OCStatus = OCS_TIME_CLOSE_START;
						OCS_Control[OCS_NUM9].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM9].mTMR_1Sec);
						break;
					case OCS_RUNNING :
						break;
				}
				switch (OCS_Control[OCS_NUM10].OCSCmd)
				{
					case OCS_STOP :
						OCS_Control[OCS_NUM10].OCStatus = OCS_RUN_STOP;
						break;
					case OCS_OPEN :
						OCS_Control[OCS_NUM10].OCStatus = OCS_OPEN_START;
						OCS_Control[OCS_NUM10].OCSCmd = OCS_RUNNING;
						break;
					case OCS_CLOSE :
						OCS_Control[OCS_NUM10].OCStatus = OCS_CLOSE_START;
						OCS_Control[OCS_NUM10].OCSCmd = OCS_RUNNING;
						break;
					case OCS_TIMED_OPEN :
						OCS_Control[OCS_NUM10].OCStatus = OCS_TIME_OPEN_START;
						OCS_Control[OCS_NUM10].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM10].mTMR_1Sec);
						break;
					case OCS_TIMED_CLOSE :
						OCS_Control[OCS_NUM10].OCStatus = OCS_TIME_CLOSE_START;
						OCS_Control[OCS_NUM10].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM10].mTMR_1Sec);
						break;
					case OCS_RUNNING :
						break;
				}
				switch (OCS_Control[OCS_NUM11].OCSCmd)
				{
					case OCS_STOP :
						OCS_Control[OCS_NUM11].OCStatus = OCS_RUN_STOP;
						break;
					case OCS_OPEN :
						OCS_Control[OCS_NUM11].OCStatus = OCS_OPEN_START;
						OCS_Control[OCS_NUM11].OCSCmd = OCS_RUNNING;
						break;
					case OCS_CLOSE :
						OCS_Control[OCS_NUM11].OCStatus = OCS_CLOSE_START;
						OCS_Control[OCS_NUM11].OCSCmd = OCS_RUNNING;
						break;
					case OCS_TIMED_OPEN :
						OCS_Control[OCS_NUM11].OCStatus = OCS_TIME_OPEN_START;
						OCS_Control[OCS_NUM11].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM11].mTMR_1Sec);
						break;
					case OCS_TIMED_CLOSE :
						OCS_Control[OCS_NUM11].OCStatus = OCS_TIME_CLOSE_START;
						OCS_Control[OCS_NUM11].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM11].mTMR_1Sec);
						break;
					case OCS_RUNNING :
						break;
				}
				switch (OCS_Control[OCS_NUM12].OCSCmd)
				{
					case OCS_STOP :
						OCS_Control[OCS_NUM12].OCStatus = OCS_RUN_STOP;
						break;
					case OCS_OPEN :
						OCS_Control[OCS_NUM12].OCStatus = OCS_OPEN_START;
						OCS_Control[OCS_NUM12].OCSCmd = OCS_RUNNING;
						break;
					case OCS_CLOSE :
						OCS_Control[OCS_NUM12].OCStatus = OCS_CLOSE_START;
						OCS_Control[OCS_NUM12].OCSCmd = OCS_RUNNING;
						break;
					case OCS_TIMED_OPEN :
						OCS_Control[OCS_NUM12].OCStatus = OCS_TIME_OPEN_START;
						OCS_Control[OCS_NUM12].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM12].mTMR_1Sec);
						break;
					case OCS_TIMED_CLOSE :
						OCS_Control[OCS_NUM12].OCStatus = OCS_TIME_CLOSE_START;
						OCS_Control[OCS_NUM12].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM12].mTMR_1Sec);
						break;
					case OCS_RUNNING :
						break;
				}
				switch (OCS_Control[OCS_NUM13].OCSCmd)
				{
					case OCS_STOP :
						OCS_Control[OCS_NUM13].OCStatus = OCS_RUN_STOP;
						break;
					case OCS_OPEN :
						OCS_Control[OCS_NUM13].OCStatus = OCS_OPEN_START;
						OCS_Control[OCS_NUM13].OCSCmd = OCS_RUNNING;
						break;
					case OCS_CLOSE :
						OCS_Control[OCS_NUM13].OCStatus = OCS_CLOSE_START;
						OCS_Control[OCS_NUM13].OCSCmd = OCS_RUNNING;
						break;
					case OCS_TIMED_OPEN :
						OCS_Control[OCS_NUM13].OCStatus = OCS_TIME_OPEN_START;
						OCS_Control[OCS_NUM13].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM13].mTMR_1Sec);
						break;
					case OCS_TIMED_CLOSE :
						OCS_Control[OCS_NUM13].OCStatus = OCS_TIME_CLOSE_START;
						OCS_Control[OCS_NUM13].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM13].mTMR_1Sec);
						break;
					case OCS_RUNNING :
						break;
				}
				switch (OCS_Control[OCS_NUM14].OCSCmd)
				{
					case OCS_STOP :
						OCS_Control[OCS_NUM14].OCStatus = OCS_RUN_STOP;
						break;
					case OCS_OPEN :
						OCS_Control[OCS_NUM14].OCStatus = OCS_OPEN_START;
						OCS_Control[OCS_NUM14].OCSCmd = OCS_RUNNING;
						break;
					case OCS_CLOSE :
						OCS_Control[OCS_NUM14].OCStatus = OCS_CLOSE_START;
						OCS_Control[OCS_NUM14].OCSCmd = OCS_RUNNING;
						break;
					case OCS_TIMED_OPEN :
						OCS_Control[OCS_NUM14].OCStatus = OCS_TIME_OPEN_START;
						OCS_Control[OCS_NUM14].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM14].mTMR_1Sec);
						break;
					case OCS_TIMED_CLOSE :
						OCS_Control[OCS_NUM14].OCStatus = OCS_TIME_CLOSE_START;
						OCS_Control[OCS_NUM14].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM14].mTMR_1Sec);
						break;
					case OCS_RUNNING :
						break;
				}
				switch (OCS_Control[OCS_NUM15].OCSCmd)
				{
					case OCS_STOP :
						OCS_Control[OCS_NUM15].OCStatus = OCS_RUN_STOP;
						break;
					case OCS_OPEN :
						OCS_Control[OCS_NUM15].OCStatus = OCS_OPEN_START;
						OCS_Control[OCS_NUM15].OCSCmd = OCS_RUNNING;
						break;
					case OCS_CLOSE :
						OCS_Control[OCS_NUM15].OCStatus = OCS_CLOSE_START;
						OCS_Control[OCS_NUM15].OCSCmd = OCS_RUNNING;
						break;
					case OCS_TIMED_OPEN :
						OCS_Control[OCS_NUM15].OCStatus = OCS_TIME_OPEN_START;
						OCS_Control[OCS_NUM15].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM15].mTMR_1Sec);
						break;
					case OCS_TIMED_CLOSE :
						OCS_Control[OCS_NUM15].OCStatus = OCS_TIME_CLOSE_START;
						OCS_Control[OCS_NUM15].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM15].mTMR_1Sec);
						break;
					case OCS_RUNNING :
						break;
				}
				switch (OCS_Control[OCS_NUM16].OCSCmd)
				{
					case OCS_STOP :
						OCS_Control[OCS_NUM16].OCStatus = OCS_RUN_STOP;
						break;
					case OCS_OPEN :
						OCS_Control[OCS_NUM16].OCStatus = OCS_OPEN_START;
						OCS_Control[OCS_NUM16].OCSCmd = OCS_RUNNING;
						break;
					case OCS_CLOSE :
						OCS_Control[OCS_NUM16].OCStatus = OCS_CLOSE_START;
						OCS_Control[OCS_NUM16].OCSCmd = OCS_RUNNING;
						break;
					case OCS_TIMED_OPEN :
						OCS_Control[OCS_NUM16].OCStatus = OCS_TIME_OPEN_START;
						OCS_Control[OCS_NUM16].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM16].mTMR_1Sec);
						break;
					case OCS_TIMED_CLOSE :
						OCS_Control[OCS_NUM16].OCStatus = OCS_TIME_CLOSE_START;
						OCS_Control[OCS_NUM16].OCSCmd = OCS_RUNNING;
						sec_timer_reset(OCS_Control[OCS_NUM16].mTMR_1Sec);
						break;
					case OCS_RUNNING :
						break;
				}
				Actuator_OCS_EXT_Next_Step(OCS_EXT_RELAY_STEP_IDLE);
			}
			else if(Actuator_OCS_EXT_Control.State == ACTUATOR_SEND_ERROR || Actuator_OCS_EXT_Control.nWaitTime >= ACT_SKIP_DELAY)
			{
				Actuator_OCS_EXT_Next_Step(Actuator_OCS_EXT_Control.Step-1);
				Actuator_OCS_EXT_Control.nRetryCount++;

				if(Actuator_OCS_EXT_Control.nRetryCount >= ACT_RETRY_CNT)
				{
					Actuator_OCS_EXT_Next_Step(OCS_EXT_RELAY_STEP_IDLE);
				}
			}
			break;
		case OCS_EXT_RELAY_STEP_IDLE :					// 7
			Actuator_OCS_EXT_Control.RelayState = RELAY_READY;
			break;
	}
}

void Actuator_OCS_EXT_Next_Step(OCS_EXT_RELAY_STEP step)
{
	Actuator_OCS_EXT_Control.Step = step;
	Actuator_OCS_EXT_Control.nWaitTime = 0;
	Actuator_OCS_EXT_Control.State = ACTUATOR_UNKNONE;

	Actuator_Ring_Buffer.nProcessIndex = Actuator_Ring_Buffer.nSaveIndex = 0;
}

//=========================================================================================
void Actuator_SW_Relay_Control_Proecess(void)
{

#if ACTUATOR_DEBUG
	static uint8_t nTempStep = 0xFF;

    if(nTempStep != Actuator_SW_Control.Step)
    {
        nTempStep = Actuator_SW_Control.Step;

        Debug_printf("\nStep = %d", nTempStep);
    }
#endif

	switch(Actuator_SW_Control.Step)
	{
		case SW_RELAY_STEP_START :           		// 0
			Actuator_SW_Next_Step(SW_RELAY_STEP_WRITE_CMD);
			break;

//===================================================================
		case SW_RELAY_STEP_WRITE_CMD :				// 1
			Actuator_SW_Control.RelayState = RELAY_BUSY;
			Actuator_SW_Control.nRetryCount = 0;

			Actuator_SW_Next_Step(SW_RELAY_STEP_STATUS_READ);
			break;

		case SW_RELAY_STEP_STATUS_READ :			// 2
			Actuator_SW_Relay_Get_Tx_Data();

			Actuator_SW_Next_Step(SW_RELAY_STEP_STATUS_READ_WAIT);
			break;

		case SW_RELAY_STEP_STATUS_READ_WAIT :		// 3
			if(Actuator_SW_Control.State == ACTUATOR_RECV_OK)
			{
				Debug_printf(" SW nReadRelay : %02X", Actuator_SW_Control.nReadRelay);

				Actuator_SW_Next_Step(SW_RELAY_STEP_WRITE_DATA_MASK);
			}
			else if(Actuator_SW_Control.State == ACTUATOR_RECV_ERROR || Actuator_SW_Control.nWaitTime >= ACT_SKIP_DELAY)
			{
				Actuator_SW_Next_Step(Actuator_SW_Control.Step - 1);
				Actuator_SW_Control.nRetryCount++;

				if(Actuator_SW_Control.nRetryCount >= ACT_RETRY_CNT)
				{
					Actuator_SW_Next_Step(SW_RELAY_STEP_IDLE);
				}
			}
			break;
		case SW_RELAY_STEP_WRITE_DATA_MASK :		// 4
			switch (SW_Control[SW_NUM1].SWCmd)
			{
				case SW_STOP :
					BITisCLEAR(Actuator_SW_Control.nReadRelay, 0);
					break;
				case SW_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 0);
					break;
				case SW_TIMED_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 0);
					break;
				case SW_RUNNING :
					break;
			}
			switch (SW_Control[SW_NUM2].SWCmd)
			{
				case SW_STOP :
					BITisCLEAR(Actuator_SW_Control.nReadRelay, 1);
					break;
				case SW_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 1);
					break;
				case SW_TIMED_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 1);
					break;
				case SW_RUNNING :
					break;
			}
			switch (SW_Control[SW_NUM3].SWCmd)
			{
				case SW_STOP :
					BITisCLEAR(Actuator_SW_Control.nReadRelay, 2);
					break;
				case SW_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 2);
					break;
				case SW_TIMED_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 2);
					break;
				case SW_RUNNING :
					break;
			}
			switch (SW_Control[SW_NUM4].SWCmd)
			{
				case SW_STOP :
					BITisCLEAR(Actuator_SW_Control.nReadRelay, 3);
					break;
				case SW_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 3);
					break;
				case SW_TIMED_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 3);
					break;
				case SW_RUNNING :
					break;
			}
			switch (SW_Control[SW_NUM5].SWCmd)
			{
				case SW_STOP :
					BITisCLEAR(Actuator_SW_Control.nReadRelay, 4);
					break;
				case SW_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 4);
					break;
				case SW_TIMED_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 4);
					break;
				case SW_RUNNING :
					break;
			}
			switch (SW_Control[SW_NUM6].SWCmd)
			{
				case SW_STOP :
					BITisCLEAR(Actuator_SW_Control.nReadRelay, 5);
					break;
				case SW_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 5);
					break;
				case SW_TIMED_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 5);
					break;
				case SW_RUNNING :
					break;
			}
			switch (SW_Control[SW_NUM7].SWCmd)
			{
				case SW_STOP :
					BITisCLEAR(Actuator_SW_Control.nReadRelay, 6);
					break;
				case SW_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 6);
					break;
				case SW_TIMED_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 6);
					break;
				case SW_RUNNING :
					break;
			}
			switch (SW_Control[SW_NUM8].SWCmd)
			{
				case SW_STOP :
					BITisCLEAR(Actuator_SW_Control.nReadRelay, 7);
					break;
				case SW_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 7);
					break;
				case SW_TIMED_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 7);
					break;
				case SW_RUNNING :
					break;
			}
			switch (SW_Control[SW_NUM9].SWCmd)
			{
				case SW_STOP :
					BITisCLEAR(Actuator_SW_Control.nReadRelay, 8);
					break;
				case SW_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 8);
					break;
				case SW_TIMED_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 8);
					break;
				case SW_RUNNING :
					break;
			}
			switch (SW_Control[SW_NUM10].SWCmd)
			{
				case SW_STOP :
					BITisCLEAR(Actuator_SW_Control.nReadRelay, 9);
					break;
				case SW_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 9);
					break;
				case SW_TIMED_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 9);
					break;
				case SW_RUNNING :
					break;
			}
			switch (SW_Control[SW_NUM11].SWCmd)
			{
				case SW_STOP :
					BITisCLEAR(Actuator_SW_Control.nReadRelay, 10);
					break;
				case SW_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 10);
					break;
				case SW_TIMED_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 10);
					break;
				case SW_RUNNING :
					break;
			}
			switch (SW_Control[SW_NUM12].SWCmd)
			{
				case SW_STOP :
					BITisCLEAR(Actuator_SW_Control.nReadRelay, 11);
					break;
				case SW_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 11);
					break;
				case SW_TIMED_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 11);
					break;
				case SW_RUNNING :
					break;
			}
			switch (SW_Control[SW_NUM13].SWCmd)
			{
				case SW_STOP :
					BITisCLEAR(Actuator_SW_Control.nReadRelay, 12);
					break;
				case SW_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 12);
					break;
				case SW_TIMED_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 12);
					break;
				case SW_RUNNING :
					break;
			}
			switch (SW_Control[SW_NUM14].SWCmd)
			{
				case SW_STOP :
					BITisCLEAR(Actuator_SW_Control.nReadRelay, 13);
					break;
				case SW_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 13);
					break;
				case SW_TIMED_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 13);
					break;
				case SW_RUNNING :
					break;
			}
			switch (SW_Control[SW_NUM15].SWCmd)
			{
				case SW_STOP :
					BITisCLEAR(Actuator_SW_Control.nReadRelay, 14);
					break;
				case SW_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 14);
					break;
				case SW_TIMED_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 14);
					break;
				case SW_RUNNING :
					break;
			}
			switch (SW_Control[SW_NUM16].SWCmd)
			{
				case SW_STOP :
					BITisCLEAR(Actuator_SW_Control.nReadRelay, 15);
					break;
				case SW_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 15);
					break;
				case SW_TIMED_ON :
					BITisSET(Actuator_SW_Control.nReadRelay, 15);
					break;
				case SW_RUNNING :
					break;
			}
			Actuator_SW_Control.nWriteRelay = Actuator_SW_Control.nReadRelay;

			Actuator_SW_Next_Step(SW_RELAY_STEP_DATA_WRITE);
			break;

		case SW_RELAY_STEP_DATA_WRITE :			// 5
			Debug_printf(" SW nWriteRelay : %02X\n",Actuator_SW_Control.nWriteRelay);
			Actuator_SW_Relay_Set_Tx_Data(Actuator_SW_Control.nWriteRelay);

			Actuator_SW_Next_Step(SW_RELAY_STEP_DATA_WRITE_WAIT);
			break;

		case SW_RELAY_STEP_DATA_WRITE_WAIT :		// 6
			if(Actuator_SW_Control.State == ACTUATOR_SEND_OK)
			{
				switch (SW_Control[SW_NUM1].SWCmd)
				{
					case SW_STOP :
						SW_Control[SW_NUM1].SWStatus = SW_OFF;
						break;
					case SW_ON :
						SW_Control[SW_NUM1].SWStatus = SW_ON_START;
						SW_Control[SW_NUM1].SWCmd = SW_RUNNING;
						break;
					case SW_TIMED_ON :
						SW_Control[SW_NUM1].SWStatus = SW_TIME_ON_START;
						SW_Control[SW_NUM1].SWCmd = SW_RUNNING;
						sec_timer_reset(SW_Control[SW_NUM1].mTMR_1Sec);
						break;
					case SW_RUNNING :
						break;
				}
				switch (SW_Control[SW_NUM2].SWCmd)
				{
					case SW_STOP :
						SW_Control[SW_NUM2].SWStatus = SW_OFF;
						break;
					case SW_ON :
						SW_Control[SW_NUM2].SWStatus = SW_ON_START;
						SW_Control[SW_NUM2].SWCmd = SW_RUNNING;
						break;
					case SW_TIMED_ON :
						SW_Control[SW_NUM2].SWStatus = SW_TIME_ON_START;
						SW_Control[SW_NUM2].SWCmd = SW_RUNNING;
						sec_timer_reset(SW_Control[SW_NUM2].mTMR_1Sec);
						break;
					case SW_RUNNING :
						break;
				}
				switch (SW_Control[SW_NUM3].SWCmd)
				{
					case SW_STOP :
						SW_Control[SW_NUM3].SWStatus = SW_OFF;
						break;
					case SW_ON :
						SW_Control[SW_NUM3].SWStatus = SW_ON_START;
						SW_Control[SW_NUM3].SWCmd = SW_RUNNING;
						break;
					case SW_TIMED_ON :
						SW_Control[SW_NUM3].SWStatus = SW_TIME_ON_START;
						SW_Control[SW_NUM3].SWCmd = SW_RUNNING;
						sec_timer_reset(SW_Control[SW_NUM3].mTMR_1Sec);
						break;
					case SW_RUNNING :
						break;
				}
				switch (SW_Control[SW_NUM4].SWCmd)
				{
					case SW_STOP :
						SW_Control[SW_NUM4].SWStatus = SW_OFF;
						break;
					case SW_ON :
						SW_Control[SW_NUM4].SWStatus = SW_ON_START;
						SW_Control[SW_NUM4].SWCmd = SW_RUNNING;
						break;
					case SW_TIMED_ON :
						SW_Control[SW_NUM4].SWStatus = SW_TIME_ON_START;
						SW_Control[SW_NUM4].SWCmd = SW_RUNNING;
						sec_timer_reset(SW_Control[SW_NUM4].mTMR_1Sec);
						break;
					case SW_RUNNING :
						break;
				}
				switch (SW_Control[SW_NUM5].SWCmd)
				{
					case SW_STOP :
						SW_Control[SW_NUM5].SWStatus = SW_OFF;
						break;
					case SW_ON :
						SW_Control[SW_NUM5].SWStatus = SW_ON_START;
						SW_Control[SW_NUM5].SWCmd = SW_RUNNING;
						break;
					case SW_TIMED_ON :
						SW_Control[SW_NUM5].SWStatus = SW_TIME_ON_START;
						SW_Control[SW_NUM5].SWCmd = SW_RUNNING;
						sec_timer_reset(SW_Control[SW_NUM5].mTMR_1Sec);
						break;
					case SW_RUNNING :
						break;
				}
				switch (SW_Control[SW_NUM6].SWCmd)
				{
					case SW_STOP :
						SW_Control[SW_NUM6].SWStatus = SW_OFF;
						break;
					case SW_ON :
						SW_Control[SW_NUM6].SWStatus = SW_ON_START;
						SW_Control[SW_NUM6].SWCmd = SW_RUNNING;
						break;
					case SW_TIMED_ON :
						SW_Control[SW_NUM6].SWStatus = SW_TIME_ON_START;
						SW_Control[SW_NUM6].SWCmd = SW_RUNNING;
						sec_timer_reset(SW_Control[SW_NUM6].mTMR_1Sec);
						break;
					case SW_RUNNING :
						break;
				}
				switch (SW_Control[SW_NUM7].SWCmd)
				{
					case SW_STOP :
						SW_Control[SW_NUM7].SWStatus = SW_OFF;
						break;
					case SW_ON :
						SW_Control[SW_NUM7].SWStatus = SW_ON_START;
						SW_Control[SW_NUM7].SWCmd = SW_RUNNING;
						break;
					case SW_TIMED_ON :
						SW_Control[SW_NUM7].SWStatus = SW_TIME_ON_START;
						SW_Control[SW_NUM7].SWCmd = SW_RUNNING;
						sec_timer_reset(SW_Control[SW_NUM7].mTMR_1Sec);
						break;
					case SW_RUNNING :
						break;
				}
				switch (SW_Control[SW_NUM8].SWCmd)
				{
					case SW_STOP :
						SW_Control[SW_NUM8].SWStatus = SW_OFF;
						break;
					case SW_ON :
						SW_Control[SW_NUM8].SWStatus = SW_ON_START;
						SW_Control[SW_NUM8].SWCmd = SW_RUNNING;
						break;
					case SW_TIMED_ON :
						SW_Control[SW_NUM8].SWStatus = SW_TIME_ON_START;
						SW_Control[SW_NUM8].SWCmd = SW_RUNNING;
						sec_timer_reset(SW_Control[SW_NUM8].mTMR_1Sec);
						break;
					case SW_RUNNING :
						break;
				}
				switch (SW_Control[SW_NUM9].SWCmd)
				{
					case SW_STOP :
						SW_Control[SW_NUM9].SWStatus = SW_OFF;
						break;
					case SW_ON :
						SW_Control[SW_NUM9].SWStatus = SW_ON_START;
						SW_Control[SW_NUM9].SWCmd = SW_RUNNING;
						break;
					case SW_TIMED_ON :
						SW_Control[SW_NUM9].SWStatus = SW_TIME_ON_START;
						SW_Control[SW_NUM9].SWCmd = SW_RUNNING;
						sec_timer_reset(SW_Control[SW_NUM9].mTMR_1Sec);
						break;
					case SW_RUNNING :
						break;
				}
				switch (SW_Control[SW_NUM10].SWCmd)
				{
					case SW_STOP :
						SW_Control[SW_NUM10].SWStatus = SW_OFF;
						break;
					case SW_ON :
						SW_Control[SW_NUM10].SWStatus = SW_ON_START;
						SW_Control[SW_NUM10].SWCmd = SW_RUNNING;
						break;
					case SW_TIMED_ON :
						SW_Control[SW_NUM10].SWStatus = SW_TIME_ON_START;
						SW_Control[SW_NUM10].SWCmd = SW_RUNNING;
						sec_timer_reset(SW_Control[SW_NUM10].mTMR_1Sec);
						break;
					case SW_RUNNING :
						break;
				}
				switch (SW_Control[SW_NUM11].SWCmd)
				{
					case SW_STOP :
						SW_Control[SW_NUM11].SWStatus = SW_OFF;
						break;
					case SW_ON :
						SW_Control[SW_NUM11].SWStatus = SW_ON_START;
						SW_Control[SW_NUM11].SWCmd = SW_RUNNING;
						break;
					case SW_TIMED_ON :
						SW_Control[SW_NUM11].SWStatus = SW_TIME_ON_START;
						SW_Control[SW_NUM11].SWCmd = SW_RUNNING;
						sec_timer_reset(SW_Control[SW_NUM11].mTMR_1Sec);
						break;
					case SW_RUNNING :
						break;
				}
				switch (SW_Control[SW_NUM12].SWCmd)
				{
					case SW_STOP :
						SW_Control[SW_NUM12].SWStatus = SW_OFF;
						break;
					case SW_ON :
						SW_Control[SW_NUM12].SWStatus = SW_ON_START;
						SW_Control[SW_NUM12].SWCmd = SW_RUNNING;
						break;
					case SW_TIMED_ON :
						SW_Control[SW_NUM12].SWStatus = SW_TIME_ON_START;
						SW_Control[SW_NUM12].SWCmd = SW_RUNNING;
						sec_timer_reset(SW_Control[SW_NUM12].mTMR_1Sec);
						break;
					case SW_RUNNING :
						break;
				}
				switch (SW_Control[SW_NUM13].SWCmd)
				{
					case SW_STOP :
						SW_Control[SW_NUM13].SWStatus = SW_OFF;
						break;
					case SW_ON :
						SW_Control[SW_NUM13].SWStatus = SW_ON_START;
						SW_Control[SW_NUM13].SWCmd = SW_RUNNING;
						break;
					case SW_TIMED_ON :
						SW_Control[SW_NUM13].SWStatus = SW_TIME_ON_START;
						SW_Control[SW_NUM13].SWCmd = SW_RUNNING;
						sec_timer_reset(SW_Control[SW_NUM13].mTMR_1Sec);
						break;
					case SW_RUNNING :
						break;
				}
				switch (SW_Control[SW_NUM14].SWCmd)
				{
					case SW_STOP :
						SW_Control[SW_NUM14].SWStatus = SW_OFF;
						break;
					case SW_ON :
						SW_Control[SW_NUM14].SWStatus = SW_ON_START;
						SW_Control[SW_NUM14].SWCmd = SW_RUNNING;
						break;
					case SW_TIMED_ON :
						SW_Control[SW_NUM14].SWStatus = SW_TIME_ON_START;
						SW_Control[SW_NUM14].SWCmd = SW_RUNNING;
						sec_timer_reset(SW_Control[SW_NUM14].mTMR_1Sec);
						break;
					case SW_RUNNING :
						break;
				}
				switch (SW_Control[SW_NUM15].SWCmd)
				{
					case SW_STOP :
						SW_Control[SW_NUM15].SWStatus = SW_OFF;
						break;
					case SW_ON :
						SW_Control[SW_NUM15].SWStatus = SW_ON_START;
						SW_Control[SW_NUM15].SWCmd = SW_RUNNING;
						break;
					case SW_TIMED_ON :
						SW_Control[SW_NUM15].SWStatus = SW_TIME_ON_START;
						SW_Control[SW_NUM15].SWCmd = SW_RUNNING;
						sec_timer_reset(SW_Control[SW_NUM15].mTMR_1Sec);
						break;
					case SW_RUNNING :
					break;
				}
				switch (SW_Control[SW_NUM16].SWCmd)
				{
					case SW_STOP :
						SW_Control[SW_NUM16].SWStatus = SW_OFF;
						break;
					case SW_ON :
						SW_Control[SW_NUM16].SWStatus = SW_ON_START;
						SW_Control[SW_NUM16].SWCmd = SW_RUNNING;
						break;
					case SW_TIMED_ON :
						SW_Control[SW_NUM16].SWStatus = SW_TIME_ON_START;
						SW_Control[SW_NUM16].SWCmd = SW_RUNNING;
						sec_timer_reset(SW_Control[SW_NUM16].mTMR_1Sec);
						break;
					case SW_RUNNING :
						break;
				}
				Actuator_SW_Next_Step(SW_RELAY_STEP_IDLE);
			}
			else if(Actuator_SW_Control.State == ACTUATOR_SEND_ERROR || Actuator_SW_Control.nWaitTime >= ACT_SKIP_DELAY)
			{
				Actuator_SW_Next_Step(Actuator_SW_Control.Step-1);
				Actuator_SW_Control.nRetryCount++;

				if(Actuator_SW_Control.nRetryCount >= ACT_RETRY_CNT)
				{
					Actuator_SW_Next_Step(SW_RELAY_STEP_IDLE);
				}
			}
			break;

		case SW_RELAY_STEP_IDLE :					// 7
			Actuator_SW_Control.RelayState = RELAY_READY;
			break;
	}
}

void Actuator_SW_Next_Step(SW_RELAY_STEP step)
{
	Actuator_SW_Control.Step = step;
	Actuator_SW_Control.nWaitTime = 0;
	Actuator_SW_Control.State = ACTUATOR_UNKNONE;

	Actuator_Ring_Buffer.nProcessIndex = Actuator_Ring_Buffer.nSaveIndex = 0;
}

void Actuator_1ms_Process(void)
{
    if(Actuator_OCS_Control.nWaitTime != 0xFFFFFFFF)
    {
    	Actuator_OCS_Control.nWaitTime++;
    }

    if(Actuator_OCS_EXT_Control.nWaitTime != 0xFFFFFFFF)
    {
    	Actuator_OCS_EXT_Control.nWaitTime++;
    }

    if(Actuator_SW_Control.nWaitTime != 0xFFFFFFFF)
    {
    	Actuator_SW_Control.nWaitTime++;
    }
}

//==================================================================================
void Debug_PutStr(char* pString)
{
    HAL_UART_Transmit(&huart4, (uint8_t *)pString, strlen(pString), 1000);
}

void Debug_printf(const char* fmt, ...)
{
	char buffer[100];
	va_list argptr;

	va_start(argptr, fmt);
	vsprintf(buffer, fmt, argptr);
	va_end(argptr);

	Debug_PutStr(buffer);
}
