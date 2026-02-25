/*
 * Master_Process.c
 *
 *  Created on: 2021. 7. 19.
 *      Author: CoreSolution
 */

#include "Master_Process.h"
#include "Actuator_Process.h"
#include "eeprom.h"
#include "tick_timer.h"

extern uint8_t MstCRC1, MstCRC2;
extern UART_HandleTypeDef huart1;

void Master_RS485_Push_Data(uint8_t nData)									// 통신 수신버퍼 입력 처리 구간
{
	Master_Ring_Buffer.RxBuffer[Master_Ring_Buffer.nSaveIndex++] = nData;

    if(Master_Ring_Buffer.nSaveIndex >= UART_RX_RING_BUFFER_SIZE)
    {
    	Master_Ring_Buffer.nSaveIndex = 0;
    }
}

uint8_t Master_RS485_Pop_Data(void)											// 통신 수신버퍼 출력 처리 구간
{
    uint8_t nRet;

    nRet = Master_Ring_Buffer.RxBuffer[Master_Ring_Buffer.nProcessIndex++];

    if(Master_Ring_Buffer.nProcessIndex >= UART_RX_RING_BUFFER_SIZE)
    {
    	Master_Ring_Buffer.nProcessIndex = 0;
    }
    return nRet;
}

void Master_NodeData_Initialize(void)  // 노드 초기화처리
{
	memset(ActNode.NodeReg, 0x0000, DEFAULT_REGISTER_MAX * sizeof(uint16_t));

	ActNode.NodeReg[1] = 0;
	ActNode.NodeReg[2] = 0;
	ActNode.NodeReg[3] = 2;											// product type : Sensor Node = 1, Actuator = 2
	ActNode.NodeReg[4] = 0;											// product code

	ActNode.NodeReg[5] = 10;										// default protocol version  "fixed"
	if(NodeVersion == KSX_TEST)
	{
		ActNode.NodeReg[6] = 24;									// default channel count   sw:16ch os:8ch
	}
	else
	{
		ActNode.NodeReg[6] = 32;									// Extend Open_Close  channel count     sw:16ch, os:16ch
	}
	ActNode.NodeReg[7] = 2510;										// Node Serial Number High : format   high byte = year, low byte= month
	ActNode.NodeReg[8] = 10;										// Node Serial Number Low   format  1word(2byte)=  update count

	ActNode.NodeReg[CON_ACTNODE_CMD_ADDR] = 1;						// 1 로컬제어 / 2리모트제어/ 3 수동제어
	ActNode.NodeReg[CON_ACTNODE_OPID_0_ADDR] = 0;					// [recheck]

	SW_Control[SW_NUM1].preOPID = 0;
	SW_Control[SW_NUM2].preOPID = 0;
	SW_Control[SW_NUM3].preOPID = 0;
	SW_Control[SW_NUM4].preOPID = 0;
	SW_Control[SW_NUM5].preOPID = 0;
	SW_Control[SW_NUM6].preOPID = 0;
	SW_Control[SW_NUM7].preOPID = 0;
	SW_Control[SW_NUM8].preOPID = 0;
	SW_Control[SW_NUM9].preOPID = 0;
	SW_Control[SW_NUM10].preOPID = 0;
	SW_Control[SW_NUM11].preOPID = 0;
	SW_Control[SW_NUM12].preOPID = 0;
	SW_Control[SW_NUM13].preOPID = 0;
	SW_Control[SW_NUM14].preOPID = 0;
	SW_Control[SW_NUM15].preOPID = 0;
	SW_Control[SW_NUM16].preOPID = 0;

	OCS_Control[OCS_NUM1].preOPID = 0;
	OCS_Control[OCS_NUM2].preOPID = 0;
	OCS_Control[OCS_NUM3].preOPID = 0;
	OCS_Control[OCS_NUM4].preOPID = 0;
	OCS_Control[OCS_NUM5].preOPID = 0;
	OCS_Control[OCS_NUM6].preOPID = 0;
	OCS_Control[OCS_NUM7].preOPID = 0;
	OCS_Control[OCS_NUM8].preOPID = 0;

	if(NodeVersion != KSX_TEST)
	{
		OCS_Control[OCS_NUM9].preOPID = 0;
		OCS_Control[OCS_NUM10].preOPID = 0;
		OCS_Control[OCS_NUM11].preOPID = 0;
		OCS_Control[OCS_NUM12].preOPID = 0;
		OCS_Control[OCS_NUM13].preOPID = 0;
		OCS_Control[OCS_NUM14].preOPID = 0;
		OCS_Control[OCS_NUM15].preOPID = 0;
		OCS_Control[OCS_NUM16].preOPID = 0;
	}

	Master_Ring_Buffer.nSaveCnt = 8;
}

void Master_Buffer_Process(void)
{
    static uint8_t RxBuffer[UART_RX_RING_BUFFER_SIZE];
    static uint8_t nIndex = 0;

	if(Master_Ring_Buffer.nSaveIndex != Master_Ring_Buffer.nProcessIndex)
	{
        uint8_t RxData = Master_RS485_Pop_Data();

#if MASTER_DEBUG
        Debug_printf("%02x %d\n ", RxData, nIndex);
#endif

        if( (nIndex == 0) && (RxData == ActNode.nNodeID || RxData == 0xFF))
        {
        	RxBuffer[nIndex++] = RxData;
        }
		else if (nIndex >= 1 && nIndex < UART_RX_RING_BUFFER_SIZE)
		{
			RxBuffer[nIndex++] = RxData;
		}
		else
		{
			nIndex = 0;
		}
	}
    else if (nIndex >= 1 && ActNode.nRxEndTime == 0 )
    {
    	RxBuffer[nIndex] = '\0';
    	Master_Ring_Buffer.nSaveCnt = nIndex;
    	nIndex = 0;

		Master_Packet_Process(RxBuffer, Master_Ring_Buffer.nSaveCnt);
		Master_Ring_Buffer.nSaveCnt = 0;
	}
}

void Master_Packet_Process(uint8_t* pPacket, uint16_t nSize)
{
	uint32_t 	rData = 0;
	uint16_t	tempAddr = 0;

#if MASTER_DEBUG
	uint8_t cnt;

	Debug_printf("  Packet_receive Ok!!!\r\n");

	for(cnt = 0; cnt < nSize; cnt++) {
		Debug_printf(" %02X\r\n", pPacket[cnt]);
	}
	Debug_printf("  Packet nSize = %d\r\n",nSize);
#endif

	if(nSize > 2) Master_Crc_Check((volatile uint8_t*)pPacket, nSize-2);
	else return;

	if( (MstCRC1 == pPacket[nSize-2]) && (MstCRC2 == pPacket[nSize-1]))
	{
		Debug_printf("  Packet_Process Ok!!!\r\n");

		if(pPacket[1] == MODBUS_READ_HOLDING_COMMAND && pPacket[0] == ActNode.nNodeID) {
			ActNode.nRequestAddr = (uint16_t)( (pPacket[2] << 8) + pPacket[3]);
			ActNode.nRequestDataNo = (uint16_t)( (pPacket[4] << 8) + pPacket[5]);

//			if( ActNode.nRequestDataNo >= 0x007D) {
//			if( ActNode.nRequestDataNo >= 0x0087) {
//				Debug_printf(" Quantity Over Error : %02X\r\n", pPacket[1]);
//				Master_Exception_0x03_Ack_Process(pPacket[1]);
//			}
			if((ActNode.nRequestDataNo == 0) || (ActNode.nRequestDataNo > 0x0087))	// Modbus RTU: Read Holding/Input Registers quantity = 1..125 (0x007D) 25.12.31. Fixed by diskept
			{
			    Debug_printf(" Quantity Over Error : %02X\r\n", pPacket[1]);
			    Master_Exception_0x03_Ack_Process(pPacket[1]);
			}
			else if( (ActNode.nRequestAddr + ActNode.nRequestDataNo) <= DEFAULT_REGISTER_MAX)
			{
				Debug_printf("Addr = %04x : DataNo = %04x\r\n",ActNode.nRequestAddr, ActNode.nRequestDataNo);
				Master_Node_Ack_Process(ActNode.nRequestAddr, ActNode.nRequestDataNo);
			}
			else
			{
				Debug_printf(" Max Address Over Error : %02X\r\n", pPacket[1]);
				Master_Exception_0x02_Ack_Process(pPacket[1]);
			}
		}
		else if(pPacket[1] == MODBUS_READ_INPUT_COMMAND)
		{
			ActNode.nRequestAddr = (uint16_t)( (pPacket[2] << 8) + pPacket[3]);
			ActNode.nRequestDataNo = (uint16_t)( (pPacket[4] << 8) + pPacket[5]);

//			if( ActNode.nRequestDataNo >= 0x007D) {
//				Debug_printf(" Quantity Over Error : %02X\r\n", pPacket[1]);
//				Master_Exception_0x03_Ack_Process(pPacket[1]);
//			}

			// 25.12.31. Fixed by diskept
			if( (ActNode.nRequestDataNo == 0) || (ActNode.nRequestDataNo > 0x0087) )
			{
			    Debug_printf(" Quantity Over Error : %02X\r\n", pPacket[1]);
			    Master_Exception_0x03_Ack_Process(pPacket[1]);
			}
			else
			{
				if(ActNode.nRequestAddr == 0xF001 && (pPacket[0] == ActNode.nNodeID || pPacket[0] == 0xFF))
				{
					Debug_printf(" ID Check Packet_receive Ok!!!\n");
					Master_ActNode_ID_Ack_Process();
				}
				else if(ActNode.nRequestAddr == 0xF002 && pPacket[0] == ActNode.nNodeID)
				{
					Debug_printf(" Node Baud Check Packet_receive Ok!!!\n");
					Master_ActNode_Baud_Ack_Process();
				}
				else if(ActNode.nRequestAddr == 0xF003 && pPacket[0] == ActNode.nNodeID)
				{
					Debug_printf(" Actuator baud Check Packet_receive Ok!!!\n");
					Master_Actuator_Baud_Ack_Process();
				}
				else
				{
					Debug_printf(" Max Address Over Error : %02X\r\n", pPacket[1]);
					Master_Exception_0x02_Ack_Process(pPacket[1]);
				}
			}
		}
		else if(pPacket[1] == MODBUS_WRITE_SINGLE_COMMAND && pPacket[0] == ActNode.nNodeID)
		{
			ActNode.nRequestAddr = (uint16_t)((pPacket[2] << 8) + pPacket[3]);

			if(ActNode.nRequestAddr == 0xF002)
			{
				Debug_printf(" Node Baud Change Packet_receive Ok!!!\n");
				Master_ActNode_Baud_Change_Process(pPacket, nSize);
			}
			else if(ActNode.nRequestAddr == 0xF003)
			{
				Debug_printf(" Sensor Baud Change Packet_receive Ok!!!\n");
				Master_Actuator_Baud_Change_Process(pPacket, nSize);
			}
			else if(ActNode.nRequestAddr == 0xF000)
			{
				Debug_printf(" Reset Packet_receive Ok!!!\n");
				Master_Factory_Reset_Process(pPacket, nSize);
			}
			else
			{
				Debug_printf(" Max Address Over Error : %02X\r\n", pPacket[1]);
				Master_Exception_0x02_Ack_Process(pPacket[1]);
			}
		}
		else if(pPacket[1] == MODBUS_WRITE_MULTIPLE_COMMAND && pPacket[0] == ActNode.nNodeID)
		{
			ActNode.nRequestAddr = (uint16_t)( (pPacket[2] << 8) + pPacket[3]);

			if(ActNode.nRequestAddr >= 501 && ActNode.nRequestAddr < 503)
			{
				Debug_printf("   ==>> Node Control Request!!!\n");
				switch(ActNode.nRequestAddr)
				{
				// 국번 명령  주소  바이트  워드   상태값  opid  CRC
				// 02 10 01 F5 00 02 04 00 02 00 04 9E 03
				//  0  1  2  3  4  5  6  7  8  9 10 11 12
				case CON_ACTNODE_CMD_ADDR :
					if((ActNode.NodeReg[CON_ACTNODE_OPID_0_ADDR] + 1) == (uint16_t)( (pPacket[9] << 8) + pPacket[10]))
					{
						ActNode.NodeReg[ACTNODE_STATUS_ADDR] = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);   		// 구동기노드 제어권 정보

						ActNode.NodeReg[CON_ACTNODE_CMD_ADDR] = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);  		// 구동기노드 제어권
						ActNode.NodeReg[CON_ACTNODE_OPID_0_ADDR] = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);  	// 구동기노드 opid

						ActNode.NodeReg[ACTNODE_OPID_0_ADDR] = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);  		// OPID #0  [reCheck]

						Master_Write_Multiple_Ack_Process(pPacket);                      // 동작명령 송신
					}
					else
					{
						Debug_printf(" Node #0 Control error\n");
					}
					break;
				}
			}
			else if(ActNode.nRequestAddr >= 503 && ActNode.nRequestAddr < 567)
			{
				Debug_printf("   ==>> SW Control Request!!!\n");
//========================================================================================================
				ActNode.nMultiConCount = (uint16_t)( (pPacket[4] << 8) + pPacket[5]);
				ActNode.nConCount = (uint8_t)(ActNode.nMultiConCount / 4);
				tempAddr = ActNode.nRequestAddr;

				ActNode.nRejCount = (567 - ActNode.nRequestAddr) / 4;		// ===== Max Count Check
				if(ActNode.nConCount > ActNode.nRejCount)
				{
					return;
				}

				if(ActNode.nConCount == 1)
				{
					ActNode.bConMode = SINGLE_MODE;
				}
				else if (ActNode.nConCount > 1)
				{
					ActNode.bConMode = MULTI_MODE;
				}

				for(uint8_t cnt = 0; cnt < ActNode.nConCount; cnt++)
				{
					ActNode.nRequestAddr = tempAddr;
					tempAddr += 4;
					Debug_printf("ActNode.nRequestAddr = %d : %d\n",cnt, ActNode.nRequestAddr);

//========================================================================================================
					switch(ActNode.nRequestAddr) {
					case CON_SWITCH_1_CMD_ADDR :
						if(Actuator.Switch_Num1 == INSTALLED)
						{
							if(ActNode.bConMode == SINGLE_MODE)
							{
								SW_Control[SW_NUM1].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								SW_Control[SW_NUM1].newOPID = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								SW_Control[SW_NUM1].nOprTime = rData;
							}
							else
							{
								SW_Control[SW_NUM1].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								SW_Control[SW_NUM1].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								SW_Control[SW_NUM1].nOprTime = rData;
							}
							SW_Control[SW_NUM1].nSW_Num = SW_NUM1;

							if( (SW_Control[SW_NUM1].preOPID < SW_Control[SW_NUM1].newOPID) && ( (SW_Control[SW_NUM1].nControl == CONTROL_OFF) ||
								(SW_Control[SW_NUM1].nControl == CONTROL_ON) || (SW_Control[SW_NUM1].nControl == CONTROL_TIMED_ON) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(SW_Control[SW_NUM1].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									SW_Control[SW_NUM1].preOPID = SW_Control[SW_NUM1].newOPID;
								}
								Debug_printf(" SW #1 nControl=%d OPID=%d OprTime=%d\n",
										SW_Control[SW_NUM1].nControl, SW_Control[SW_NUM1].newOPID, SW_Control[SW_NUM1].nOprTime);

								Master_Actuator_SW_Updata_Process(SW_Control[SW_NUM1]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" SW #1 Control error\n");
						}
						break;
					case CON_SWITCH_2_CMD_ADDR :
						if(Actuator.Switch_Num2 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								SW_Control[SW_NUM2].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								SW_Control[SW_NUM2].newOPID = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								SW_Control[SW_NUM2].nOprTime = rData;
							}
							else{
								SW_Control[SW_NUM2].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								SW_Control[SW_NUM2].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								SW_Control[SW_NUM2].nOprTime = rData;
							}
							SW_Control[SW_NUM2].nSW_Num = SW_NUM2;

							if( (SW_Control[SW_NUM2].preOPID < SW_Control[SW_NUM2].newOPID) && ( (SW_Control[SW_NUM2].nControl == CONTROL_OFF) ||
								(SW_Control[SW_NUM2].nControl == CONTROL_ON) || (SW_Control[SW_NUM2].nControl == CONTROL_TIMED_ON) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(SW_Control[SW_NUM2].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									SW_Control[SW_NUM2].preOPID = SW_Control[SW_NUM2].newOPID;
								}
								Debug_printf(" SW #2 nControl=%d OPID=%d OprTime=%d\n",
										SW_Control[SW_NUM2].nControl, SW_Control[SW_NUM2].newOPID, SW_Control[SW_NUM2].nOprTime);

								Master_Actuator_SW_Updata_Process(SW_Control[SW_NUM2]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" SW #2 Control error\n");
						}
						break;
					case CON_SWITCH_3_CMD_ADDR :
						if(Actuator.Switch_Num3 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								SW_Control[SW_NUM3].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								SW_Control[SW_NUM3].newOPID = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								SW_Control[SW_NUM3].nOprTime = rData;
							}
							else{
								SW_Control[SW_NUM3].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								SW_Control[SW_NUM3].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								SW_Control[SW_NUM3].nOprTime = rData;
							}
							SW_Control[SW_NUM3].nSW_Num = SW_NUM3;

							if( (SW_Control[SW_NUM3].preOPID < SW_Control[SW_NUM3].newOPID) && ( (SW_Control[SW_NUM3].nControl == CONTROL_OFF) ||
								(SW_Control[SW_NUM3].nControl == CONTROL_ON) || (SW_Control[SW_NUM3].nControl == CONTROL_TIMED_ON) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(SW_Control[SW_NUM3].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									SW_Control[SW_NUM3].preOPID = SW_Control[SW_NUM3].newOPID;
								}
								Debug_printf(" SW #3 nControl=%d OPID=%d OprTime=%d\n",
										SW_Control[SW_NUM3].nControl, SW_Control[SW_NUM3].newOPID, SW_Control[SW_NUM3].nOprTime);

								Master_Actuator_SW_Updata_Process(SW_Control[SW_NUM3]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" SW #3 Control error\n");
						}
						break;
					case CON_SWITCH_4_CMD_ADDR :
						if(Actuator.Switch_Num4 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								SW_Control[SW_NUM4].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								SW_Control[SW_NUM4].newOPID = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								SW_Control[SW_NUM4].nOprTime = rData;
							}
							else{
								SW_Control[SW_NUM4].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								SW_Control[SW_NUM4].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								SW_Control[SW_NUM4].nOprTime = rData;
							}
							SW_Control[SW_NUM4].nSW_Num = SW_NUM4;

							if( (SW_Control[SW_NUM4].preOPID < SW_Control[SW_NUM4].newOPID) && ( (SW_Control[SW_NUM4].nControl == CONTROL_OFF) ||
								(SW_Control[SW_NUM4].nControl == CONTROL_ON) || (SW_Control[SW_NUM4].nControl == CONTROL_TIMED_ON)) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(SW_Control[SW_NUM4].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									SW_Control[SW_NUM4].preOPID = SW_Control[SW_NUM4].newOPID;
								}
								Debug_printf(" SW #4 nControl=%d OPID=%d OprTime=%d\n",
										SW_Control[SW_NUM4].nControl, SW_Control[SW_NUM4].newOPID, SW_Control[SW_NUM4].nOprTime);

								Master_Actuator_SW_Updata_Process(SW_Control[SW_NUM4]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" SW #4 Control error\n");
						}
						break;
					case CON_SWITCH_5_CMD_ADDR :
						if(Actuator.Switch_Num5 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								SW_Control[SW_NUM5].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								SW_Control[SW_NUM5].newOPID = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								SW_Control[SW_NUM5].nOprTime = rData;
							}
							else{
								SW_Control[SW_NUM5].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								SW_Control[SW_NUM5].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								SW_Control[SW_NUM5].nOprTime = rData;
							}
							SW_Control[SW_NUM5].nSW_Num = SW_NUM5;

							if( (SW_Control[SW_NUM5].preOPID < SW_Control[SW_NUM5].newOPID) && ( (SW_Control[SW_NUM5].nControl == CONTROL_OFF) ||
								(SW_Control[SW_NUM5].nControl == CONTROL_ON) || (SW_Control[SW_NUM5].nControl == CONTROL_TIMED_ON) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(SW_Control[SW_NUM5].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									SW_Control[SW_NUM5].preOPID = SW_Control[SW_NUM5].newOPID;
								}
								Debug_printf(" SW #5 nControl=%d OPID=%d OprTime=%d\n",
										SW_Control[SW_NUM5].nControl, SW_Control[SW_NUM5].newOPID, SW_Control[SW_NUM5].nOprTime);

								Master_Actuator_SW_Updata_Process(SW_Control[SW_NUM5]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" SW #5 Control error\n");
						}
						break;
					case CON_SWITCH_6_CMD_ADDR :
						if(Actuator.Switch_Num6 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								SW_Control[SW_NUM6].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								SW_Control[SW_NUM6].newOPID = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								SW_Control[SW_NUM6].nOprTime = rData;
							}
							else{
								SW_Control[SW_NUM6].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								SW_Control[SW_NUM6].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								SW_Control[SW_NUM6].nOprTime = rData;
							}
							SW_Control[SW_NUM6].nSW_Num = SW_NUM6;

							if( (SW_Control[SW_NUM6].preOPID < SW_Control[SW_NUM6].newOPID) && ( (SW_Control[SW_NUM6].nControl == CONTROL_OFF) ||
								(SW_Control[SW_NUM6].nControl == CONTROL_ON) || (SW_Control[SW_NUM6].nControl == CONTROL_TIMED_ON) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(SW_Control[SW_NUM6].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									SW_Control[SW_NUM6].preOPID = SW_Control[SW_NUM6].newOPID;
								}
								Debug_printf(" SW #6 nControl=%d OPID=%d OprTime=%d\n",
										SW_Control[SW_NUM6].nControl, SW_Control[SW_NUM6].newOPID, SW_Control[SW_NUM6].nOprTime);

								Master_Actuator_SW_Updata_Process(SW_Control[SW_NUM6]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" SW #6 Control error\n");
						}
						break;
					case CON_SWITCH_7_CMD_ADDR :
						if(Actuator.Switch_Num7 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								SW_Control[SW_NUM7].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								SW_Control[SW_NUM7].newOPID = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								SW_Control[SW_NUM7].nOprTime = rData;
							}
							else{
								SW_Control[SW_NUM7].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								SW_Control[SW_NUM7].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								SW_Control[SW_NUM7].nOprTime = rData;
							}
							SW_Control[SW_NUM7].nSW_Num = SW_NUM7;

							if( (SW_Control[SW_NUM7].preOPID < SW_Control[SW_NUM7].newOPID) && ( (SW_Control[SW_NUM7].nControl == CONTROL_OFF) ||
								(SW_Control[SW_NUM7].nControl == CONTROL_ON) || (SW_Control[SW_NUM7].nControl == CONTROL_TIMED_ON) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(SW_Control[SW_NUM7].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									SW_Control[SW_NUM7].preOPID = SW_Control[SW_NUM7].newOPID;
								}
								Debug_printf(" SW #7 nControl=%d OPID=%d OprTime=%d\n",
										SW_Control[SW_NUM7].nControl, SW_Control[SW_NUM7].newOPID, SW_Control[SW_NUM7].nOprTime);

								Master_Actuator_SW_Updata_Process(SW_Control[SW_NUM7]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" SW #7 Control error\n");
						}
						break;
					case CON_SWITCH_8_CMD_ADDR :
						if(Actuator.Switch_Num8 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								SW_Control[SW_NUM8].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								SW_Control[SW_NUM8].newOPID = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								SW_Control[SW_NUM8].nOprTime = rData;
							}
							else{
								SW_Control[SW_NUM8].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								SW_Control[SW_NUM8].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								SW_Control[SW_NUM8].nOprTime = rData;
							}
							SW_Control[SW_NUM8].nSW_Num = SW_NUM8;

							if( (SW_Control[SW_NUM8].preOPID < SW_Control[SW_NUM8].newOPID) && ( (SW_Control[SW_NUM8].nControl == CONTROL_OFF) ||
								(SW_Control[SW_NUM8].nControl == CONTROL_ON) || (SW_Control[SW_NUM8].nControl == CONTROL_TIMED_ON) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(SW_Control[SW_NUM8].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									SW_Control[SW_NUM8].preOPID = SW_Control[SW_NUM8].newOPID;
								}
								Debug_printf(" SW #8 nControl=%d OPID=%d OprTime=%d\n",
										SW_Control[SW_NUM8].nControl, SW_Control[SW_NUM8].newOPID, SW_Control[SW_NUM8].nOprTime);

								Master_Actuator_SW_Updata_Process(SW_Control[SW_NUM8]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" SW #8 Control error\n");
						}
						break;
					case CON_SWITCH_9_CMD_ADDR :
						if(Actuator.Switch_Num9 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								SW_Control[SW_NUM9].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								SW_Control[SW_NUM9].newOPID = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								SW_Control[SW_NUM9].nOprTime = rData;
							}
							else{
								SW_Control[SW_NUM9].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								SW_Control[SW_NUM9].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								SW_Control[SW_NUM9].nOprTime = rData;
							}
							SW_Control[SW_NUM9].nSW_Num = SW_NUM9;

							if( (SW_Control[SW_NUM9].preOPID < SW_Control[SW_NUM9].newOPID) && ( (SW_Control[SW_NUM9].nControl == CONTROL_OFF) ||
								(SW_Control[SW_NUM9].nControl == CONTROL_ON) || (SW_Control[SW_NUM9].nControl == CONTROL_TIMED_ON) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(SW_Control[SW_NUM9].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									SW_Control[SW_NUM9].preOPID = SW_Control[SW_NUM9].newOPID;
								}
								Debug_printf(" SW #9 nControl=%d OPID=%d OprTime=%d\n",
										SW_Control[SW_NUM9].nControl, SW_Control[SW_NUM9].newOPID, SW_Control[SW_NUM9].nOprTime);

								Master_Actuator_SW_Updata_Process(SW_Control[SW_NUM9]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" SW #9 Control error\n");
						}
						break;
					case CON_SWITCH_10_CMD_ADDR :
						if(Actuator.Switch_Num10 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								SW_Control[SW_NUM10].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								SW_Control[SW_NUM10].newOPID = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								SW_Control[SW_NUM10].nOprTime = rData;
							}
							else{
								SW_Control[SW_NUM10].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								SW_Control[SW_NUM10].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								SW_Control[SW_NUM10].nOprTime = rData;
							}
							SW_Control[SW_NUM10].nSW_Num = SW_NUM10;

							if( (SW_Control[SW_NUM10].preOPID < SW_Control[SW_NUM10].newOPID) && ( (SW_Control[SW_NUM10].nControl == CONTROL_OFF ) ||
								(SW_Control[SW_NUM10].nControl == CONTROL_ON) || (SW_Control[SW_NUM10].nControl == CONTROL_TIMED_ON) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(SW_Control[SW_NUM10].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									SW_Control[SW_NUM10].preOPID = SW_Control[SW_NUM10].newOPID;
								}
								Debug_printf("SW #10 nControl=%d OPID=%d OprTime=%d\n",
										SW_Control[SW_NUM10].nControl, SW_Control[SW_NUM10].newOPID, SW_Control[SW_NUM10].nOprTime);

								Master_Actuator_SW_Updata_Process(SW_Control[SW_NUM10]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" SW #10 Control error\n");
						}
						break;
					case CON_SWITCH_11_CMD_ADDR :
						if(Actuator.Switch_Num11 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								SW_Control[SW_NUM11].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								SW_Control[SW_NUM11].newOPID = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								SW_Control[SW_NUM11].nOprTime = rData;
							}
							else{
								SW_Control[SW_NUM11].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								SW_Control[SW_NUM11].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								SW_Control[SW_NUM11].nOprTime = rData;
							}
							SW_Control[SW_NUM11].nSW_Num = SW_NUM11;

							if( (SW_Control[SW_NUM11].preOPID < SW_Control[SW_NUM11].newOPID) && ( (SW_Control[SW_NUM11].nControl == CONTROL_OFF) ||
								(SW_Control[SW_NUM11].nControl == CONTROL_ON) || (SW_Control[SW_NUM11].nControl == CONTROL_TIMED_ON) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(SW_Control[SW_NUM11].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									SW_Control[SW_NUM11].preOPID = SW_Control[SW_NUM11].newOPID;
								}
								Debug_printf("SW #11 nControl=%d OPID=%d OprTime=%d\n",
										SW_Control[SW_NUM11].nControl, SW_Control[SW_NUM11].newOPID, SW_Control[SW_NUM11].nOprTime);

								Master_Actuator_SW_Updata_Process(SW_Control[SW_NUM11]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" SW #11 Control error\n");
						}
						break;
					case CON_SWITCH_12_CMD_ADDR :
						if(Actuator.Switch_Num12 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								SW_Control[SW_NUM12].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								SW_Control[SW_NUM12].newOPID = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								SW_Control[SW_NUM12].nOprTime = rData;
							}
							else{
								SW_Control[SW_NUM12].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								SW_Control[SW_NUM12].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								SW_Control[SW_NUM12].nOprTime = rData;
							}
							SW_Control[SW_NUM12].nSW_Num = SW_NUM12;

							if( (SW_Control[SW_NUM12].preOPID < SW_Control[SW_NUM12].newOPID) && ( (SW_Control[SW_NUM12].nControl == CONTROL_OFF) ||
								(SW_Control[SW_NUM12].nControl == CONTROL_ON) || (SW_Control[SW_NUM12].nControl == CONTROL_TIMED_ON) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(SW_Control[SW_NUM12].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									SW_Control[SW_NUM12].preOPID = SW_Control[SW_NUM12].newOPID;
								}
								Debug_printf("SW #12 nControl=%d OPID=%d OprTime=%d\n",
										SW_Control[SW_NUM12].nControl, SW_Control[SW_NUM12].newOPID, SW_Control[SW_NUM12].nOprTime);

								Master_Actuator_SW_Updata_Process(SW_Control[SW_NUM12]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" SW #12 Control error\n");
						}
						break;
					case CON_SWITCH_13_CMD_ADDR :
						if(Actuator.Switch_Num13 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								SW_Control[SW_NUM13].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								SW_Control[SW_NUM13].newOPID = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								SW_Control[SW_NUM13].nOprTime = rData;
							}
							else{
								SW_Control[SW_NUM13].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								SW_Control[SW_NUM13].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								SW_Control[SW_NUM13].nOprTime = rData;
							}
							SW_Control[SW_NUM13].nSW_Num = SW_NUM13;

							if( (SW_Control[SW_NUM13].preOPID < SW_Control[SW_NUM13].newOPID) && ( (SW_Control[SW_NUM13].nControl == CONTROL_OFF) ||
								(SW_Control[SW_NUM13].nControl == CONTROL_ON) || (SW_Control[SW_NUM13].nControl == CONTROL_TIMED_ON) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(SW_Control[SW_NUM13].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									SW_Control[SW_NUM13].preOPID = SW_Control[SW_NUM13].newOPID;
								}
								Debug_printf("SW #13 nControl=%d OPID=%d OprTime=%d\n",
										SW_Control[SW_NUM13].nControl, SW_Control[SW_NUM13].newOPID, SW_Control[SW_NUM13].nOprTime);

								Master_Actuator_SW_Updata_Process(SW_Control[SW_NUM13]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" SW #13 Control error\n");
						}
						break;
					case CON_SWITCH_14_CMD_ADDR :
						if(Actuator.Switch_Num14 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								SW_Control[SW_NUM14].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								SW_Control[SW_NUM14].newOPID = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								SW_Control[SW_NUM14].nOprTime = rData;
							}
							else{
								SW_Control[SW_NUM14].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								SW_Control[SW_NUM14].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								SW_Control[SW_NUM14].nOprTime = rData;
							}
							SW_Control[SW_NUM14].nSW_Num = SW_NUM14;

							if( (SW_Control[SW_NUM14].preOPID < SW_Control[SW_NUM14].newOPID) && ( (SW_Control[SW_NUM14].nControl == CONTROL_OFF) ||
								(SW_Control[SW_NUM14].nControl == CONTROL_ON) || (SW_Control[SW_NUM14].nControl == CONTROL_TIMED_ON) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(SW_Control[SW_NUM14].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									SW_Control[SW_NUM14].preOPID = SW_Control[SW_NUM14].newOPID;
								}
								Debug_printf("SW #14 nControl=%d OPID=%d OprTime=%d\n",
										SW_Control[SW_NUM14].nControl, SW_Control[SW_NUM14].newOPID, SW_Control[SW_NUM14].nOprTime);

								Master_Actuator_SW_Updata_Process(SW_Control[SW_NUM14]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" SW #14 Control error\n");
						}
						break;
					case CON_SWITCH_15_CMD_ADDR :
						if(Actuator.Switch_Num15 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								SW_Control[SW_NUM15].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								SW_Control[SW_NUM15].newOPID = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								SW_Control[SW_NUM15].nOprTime = rData;
							}
							else{
								SW_Control[SW_NUM15].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								SW_Control[SW_NUM15].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								SW_Control[SW_NUM15].nOprTime = rData;
							}
							SW_Control[SW_NUM15].nSW_Num = SW_NUM15;

							if( (SW_Control[SW_NUM15].preOPID < SW_Control[SW_NUM15].newOPID) && ( (SW_Control[SW_NUM15].nControl == CONTROL_OFF) ||
								(SW_Control[SW_NUM15].nControl == CONTROL_ON) || (SW_Control[SW_NUM15].nControl == CONTROL_TIMED_ON) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(SW_Control[SW_NUM15].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									SW_Control[SW_NUM15].preOPID = SW_Control[SW_NUM15].newOPID;
								}
								Debug_printf("SW #15 nControl=%d OPID=%d OprTime=%d\n",
										SW_Control[SW_NUM15].nControl, SW_Control[SW_NUM15].newOPID, SW_Control[SW_NUM15].nOprTime);

								Master_Actuator_SW_Updata_Process(SW_Control[SW_NUM15]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" SW #15 Control error\n");
						}
						break;
					case CON_SWITCH_16_CMD_ADDR :
						if(Actuator.Switch_Num16 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								SW_Control[SW_NUM16].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								SW_Control[SW_NUM16].newOPID = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								SW_Control[SW_NUM16].nOprTime = rData;
							}
							else{
								SW_Control[SW_NUM16].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								SW_Control[SW_NUM16].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								SW_Control[SW_NUM16].nOprTime = rData;
							}
							SW_Control[SW_NUM16].nSW_Num = SW_NUM16;

							if( (SW_Control[SW_NUM16].preOPID < SW_Control[SW_NUM16].newOPID) && ( (SW_Control[SW_NUM16].nControl == CONTROL_OFF) ||
								(SW_Control[SW_NUM16].nControl == CONTROL_ON) || (SW_Control[SW_NUM16].nControl == CONTROL_TIMED_ON) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(SW_Control[SW_NUM16].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									SW_Control[SW_NUM16].preOPID = SW_Control[SW_NUM16].newOPID;
								}
								Debug_printf("SW #16 nControl=%d OPID=%d OprTime=%d\n",
										SW_Control[SW_NUM16].nControl, SW_Control[SW_NUM16].newOPID, SW_Control[SW_NUM16].nOprTime);

								Master_Actuator_SW_Updata_Process(SW_Control[SW_NUM16]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" SW #16 Control error\n");
						}
						break;
					}
				}
				if(ActNode.bConMode == MULTI_MODE) Master_Write_Multiple_Ack_Process(pPacket);
			}
//========================================================================================================
			else if(ActNode.nRequestAddr >= 567 && ActNode.nRequestAddr < 599) {
				Debug_printf("   ==>> OCS Control A Request!!!\n");
//========================================================================================================
				ActNode.nMultiConCount = (uint16_t)( (pPacket[4] << 8) + pPacket[5]);
				ActNode.nConCount = (uint8_t)(ActNode.nMultiConCount / 4);
				tempAddr = ActNode.nRequestAddr;

				ActNode.nRejCount = (599 - ActNode.nRequestAddr) / 4;		// ===== Max Count Check
				if(ActNode.nConCount > ActNode.nRejCount) return;

				if(ActNode.nConCount == 1) ActNode.bConMode = SINGLE_MODE;
				else if (ActNode.nConCount > 1) ActNode.bConMode = MULTI_MODE;

				for(uint8_t cnt = 0; cnt < ActNode.nConCount; cnt++) {
					ActNode.nRequestAddr = tempAddr;
					tempAddr += 4;
					Debug_printf("ActNode.nRequestAddr = %d : %d\n",cnt, ActNode.nRequestAddr);

//========================================================================================================
					switch(ActNode.nRequestAddr) {
					case CON_OCSWITCH_1_CMD_ADDR :
						if(Actuator.OCSwitch_Num1 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								OCS_Control[OCS_NUM1].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								OCS_Control[OCS_NUM1].newOPID  = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								OCS_Control[OCS_NUM1].nOprTime = rData;
							}
							else{
								OCS_Control[OCS_NUM1].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								OCS_Control[OCS_NUM1].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								OCS_Control[OCS_NUM1].nOprTime = rData;
							}
							OCS_Control[OCS_NUM1].nOCS_Num = OCS_NUM1;

							if( (OCS_Control[OCS_NUM1].preOPID < OCS_Control[OCS_NUM1].newOPID) && ( (OCS_Control[OCS_NUM1].nControl == CONTROL_STOP) ||
								(OCS_Control[OCS_NUM1].nControl == CONTROL_OPEN) || (OCS_Control[OCS_NUM1].nControl == CONTROL_CLOSE) ||
								(OCS_Control[OCS_NUM1].nControl == CONTROL_TIMED_OPEN) || (OCS_Control[OCS_NUM1].nControl == CONTROL_TIMED_CLOSE) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(OCS_Control[OCS_NUM1].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									OCS_Control[OCS_NUM1].preOPID = OCS_Control[OCS_NUM1].newOPID;
								}
								Debug_printf(" OCS #1 nControl=%d OPID=%d OprTime=%d\n",
										OCS_Control[OCS_NUM1].nControl, OCS_Control[OCS_NUM1].newOPID, OCS_Control[OCS_NUM1].nOprTime);

								Master_Actuator_OCS_Updata_Process(OCS_Control[OCS_NUM1]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" OCS #1 Control error or Keep\n");
						}
						break;
					case CON_OCSWITCH_2_CMD_ADDR :
						if(Actuator.OCSwitch_Num2 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								OCS_Control[OCS_NUM2].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								OCS_Control[OCS_NUM2].newOPID  = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								OCS_Control[OCS_NUM2].nOprTime = rData;
							}
							else{
								OCS_Control[OCS_NUM2].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								OCS_Control[OCS_NUM2].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								OCS_Control[OCS_NUM2].nOprTime = rData;
							}
							OCS_Control[OCS_NUM2].nOCS_Num = OCS_NUM2;

							if( (OCS_Control[OCS_NUM2].preOPID < OCS_Control[OCS_NUM2].newOPID) && ( (OCS_Control[OCS_NUM2].nControl == CONTROL_STOP) ||
								(OCS_Control[OCS_NUM2].nControl == CONTROL_OPEN) || (OCS_Control[OCS_NUM2].nControl == CONTROL_CLOSE) ||
								(OCS_Control[OCS_NUM2].nControl == CONTROL_TIMED_OPEN) || (OCS_Control[OCS_NUM2].nControl == CONTROL_TIMED_CLOSE) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(OCS_Control[OCS_NUM2].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									OCS_Control[OCS_NUM2].preOPID = OCS_Control[OCS_NUM2].newOPID;
								}
								Debug_printf(" OCS #2 nControl=%d OPID=%d OprTime=%d\n",
										OCS_Control[OCS_NUM2].nControl, OCS_Control[OCS_NUM2].newOPID, OCS_Control[OCS_NUM2].nOprTime);

								Master_Actuator_OCS_Updata_Process(OCS_Control[OCS_NUM2]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" OCS #2 Control error or Keep\n");
						}
						break;
					case CON_OCSWITCH_3_CMD_ADDR :
						if(Actuator.OCSwitch_Num3 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								OCS_Control[OCS_NUM3].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								OCS_Control[OCS_NUM3].newOPID  = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								OCS_Control[OCS_NUM3].nOprTime = rData;
							}
							else{
								OCS_Control[OCS_NUM3].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								OCS_Control[OCS_NUM3].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								OCS_Control[OCS_NUM3].nOprTime = rData;
							}
							OCS_Control[OCS_NUM3].nOCS_Num = OCS_NUM3;

							if( (OCS_Control[OCS_NUM3].preOPID < OCS_Control[OCS_NUM3].newOPID) && ( (OCS_Control[OCS_NUM3].nControl == CONTROL_STOP) ||
								(OCS_Control[OCS_NUM3].nControl == CONTROL_OPEN) || (OCS_Control[OCS_NUM3].nControl == CONTROL_CLOSE) ||
								(OCS_Control[OCS_NUM3].nControl == CONTROL_TIMED_OPEN) || (OCS_Control[OCS_NUM3].nControl == CONTROL_TIMED_CLOSE) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(OCS_Control[OCS_NUM3].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									OCS_Control[OCS_NUM3].preOPID = OCS_Control[OCS_NUM3].newOPID;
								}
								Debug_printf(" OCS #3 nControl=%d OPID=%d OprTime=%d\n",
										OCS_Control[OCS_NUM3].nControl, OCS_Control[OCS_NUM3].newOPID, OCS_Control[OCS_NUM3].nOprTime);

								Master_Actuator_OCS_Updata_Process(OCS_Control[OCS_NUM3]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" OCS #3 Control error or Keep\n");

						}
						break;
					case CON_OCSWITCH_4_CMD_ADDR :
						if(Actuator.OCSwitch_Num4 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								OCS_Control[OCS_NUM4].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								OCS_Control[OCS_NUM4].newOPID  = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								OCS_Control[OCS_NUM4].nOprTime = rData;
							}
							else{
								OCS_Control[OCS_NUM4].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								OCS_Control[OCS_NUM4].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								OCS_Control[OCS_NUM4].nOprTime = rData;
							}
							OCS_Control[OCS_NUM4].nOCS_Num = OCS_NUM4;

							if( (OCS_Control[OCS_NUM4].preOPID < OCS_Control[OCS_NUM4].newOPID) && ( (OCS_Control[OCS_NUM4].nControl == CONTROL_STOP) ||
								(OCS_Control[OCS_NUM4].nControl == CONTROL_OPEN) || (OCS_Control[OCS_NUM4].nControl == CONTROL_CLOSE) ||
								(OCS_Control[OCS_NUM4].nControl == CONTROL_TIMED_OPEN) || (OCS_Control[OCS_NUM4].nControl == CONTROL_TIMED_CLOSE) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(OCS_Control[OCS_NUM4].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									OCS_Control[OCS_NUM4].preOPID = OCS_Control[OCS_NUM4].newOPID;
								}
								Debug_printf(" OCS #4 nControl=%d OPID=%d OprTime=%d\n",
										OCS_Control[OCS_NUM4].nControl, OCS_Control[OCS_NUM4].newOPID, OCS_Control[OCS_NUM4].nOprTime);

								Master_Actuator_OCS_Updata_Process(OCS_Control[OCS_NUM4]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" OCS #4 Control error or Keep\n");

						}
						break;
					case CON_OCSWITCH_5_CMD_ADDR :
						if(Actuator.OCSwitch_Num5 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								OCS_Control[OCS_NUM5].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								OCS_Control[OCS_NUM5].newOPID  = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								OCS_Control[OCS_NUM5].nOprTime = rData;
							}
							else{
								OCS_Control[OCS_NUM5].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								OCS_Control[OCS_NUM5].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								OCS_Control[OCS_NUM5].nOprTime = rData;
							}
							OCS_Control[OCS_NUM5].nOCS_Num = OCS_NUM5;

							if( (OCS_Control[OCS_NUM5].preOPID < OCS_Control[OCS_NUM5].newOPID) && ( (OCS_Control[OCS_NUM5].nControl == CONTROL_STOP) ||
							    (OCS_Control[OCS_NUM5].nControl == CONTROL_OPEN) || (OCS_Control[OCS_NUM5].nControl == CONTROL_CLOSE) ||
								(OCS_Control[OCS_NUM5].nControl == CONTROL_TIMED_OPEN) || (OCS_Control[OCS_NUM5].nControl == CONTROL_TIMED_CLOSE) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(OCS_Control[OCS_NUM5].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									OCS_Control[OCS_NUM5].preOPID = OCS_Control[OCS_NUM5].newOPID;
								}
								Debug_printf(" OCS #5 nControl=%d OPID=%d OprTime=%d\n",
										OCS_Control[OCS_NUM5].nControl, OCS_Control[OCS_NUM5].newOPID, OCS_Control[OCS_NUM5].nOprTime);

								Master_Actuator_OCS_Updata_Process(OCS_Control[OCS_NUM5]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" OCS #5 Control error or Keep\n");
						}
						break;
					case CON_OCSWITCH_6_CMD_ADDR :
						if(Actuator.OCSwitch_Num6 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								OCS_Control[OCS_NUM6].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								OCS_Control[OCS_NUM6].newOPID  = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								OCS_Control[OCS_NUM6].nOprTime = rData;
							}
							else{
								OCS_Control[OCS_NUM6].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								OCS_Control[OCS_NUM6].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								OCS_Control[OCS_NUM6].nOprTime = rData;
							}
							OCS_Control[OCS_NUM6].nOCS_Num = OCS_NUM6;

							if( (OCS_Control[OCS_NUM6].preOPID < OCS_Control[OCS_NUM6].newOPID) && ( (OCS_Control[OCS_NUM6].nControl == CONTROL_STOP) ||
								(OCS_Control[OCS_NUM6].nControl == CONTROL_OPEN) || (OCS_Control[OCS_NUM6].nControl == CONTROL_CLOSE) ||
								(OCS_Control[OCS_NUM6].nControl == CONTROL_TIMED_OPEN) || (OCS_Control[OCS_NUM6].nControl == CONTROL_TIMED_CLOSE) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(OCS_Control[OCS_NUM6].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									OCS_Control[OCS_NUM6].preOPID = OCS_Control[OCS_NUM6].newOPID;
								}
								Debug_printf(" OCS #6 nControl=%d OPID=%d OprTime=%d\n",
										OCS_Control[OCS_NUM6].nControl, OCS_Control[OCS_NUM6].newOPID, OCS_Control[OCS_NUM6].nOprTime);

								Master_Actuator_OCS_Updata_Process(OCS_Control[OCS_NUM6]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" OCS #6 Control error or Keep\n");

						}
						break;
					case CON_OCSWITCH_7_CMD_ADDR :
						if(Actuator.OCSwitch_Num7 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								OCS_Control[OCS_NUM7].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								OCS_Control[OCS_NUM7].newOPID  = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								OCS_Control[OCS_NUM7].nOprTime = rData;
							}
							else{
								OCS_Control[OCS_NUM7].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								OCS_Control[OCS_NUM7].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								OCS_Control[OCS_NUM7].nOprTime = rData;
							}
							OCS_Control[OCS_NUM7].nOCS_Num = OCS_NUM7;

							if( (OCS_Control[OCS_NUM7].preOPID < OCS_Control[OCS_NUM7].newOPID) && ( (OCS_Control[OCS_NUM7].nControl == CONTROL_STOP) ||
								(OCS_Control[OCS_NUM7].nControl == CONTROL_OPEN) || (OCS_Control[OCS_NUM7].nControl == CONTROL_CLOSE) ||
								(OCS_Control[OCS_NUM7].nControl == CONTROL_TIMED_OPEN) || (OCS_Control[OCS_NUM7].nControl == CONTROL_TIMED_CLOSE) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(OCS_Control[OCS_NUM7].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									OCS_Control[OCS_NUM7].preOPID = OCS_Control[OCS_NUM7].newOPID;
								}
								Debug_printf(" OCS #7 nControl=%d OPID=%d OprTime=%d\n",
										OCS_Control[OCS_NUM7].nControl, OCS_Control[OCS_NUM7].newOPID, OCS_Control[OCS_NUM7].nOprTime);

								Master_Actuator_OCS_Updata_Process(OCS_Control[OCS_NUM7]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" OCS #7 Control error or Keep\n");

						}
						break;
					case CON_OCSWITCH_8_CMD_ADDR :
						if(Actuator.OCSwitch_Num8 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								OCS_Control[OCS_NUM8].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								OCS_Control[OCS_NUM8].newOPID  = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								OCS_Control[OCS_NUM8].nOprTime = rData;
							}
							else{
								OCS_Control[OCS_NUM8].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								OCS_Control[OCS_NUM8].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								OCS_Control[OCS_NUM8].nOprTime = rData;
							}
							OCS_Control[OCS_NUM8].nOCS_Num = OCS_NUM8;

							if( (OCS_Control[OCS_NUM8].preOPID < OCS_Control[OCS_NUM8].newOPID) &&( (OCS_Control[OCS_NUM8].nControl == CONTROL_STOP) ||
								(OCS_Control[OCS_NUM8].nControl == CONTROL_OPEN) || (OCS_Control[OCS_NUM8].nControl == CONTROL_CLOSE) ||
								(OCS_Control[OCS_NUM8].nControl == CONTROL_TIMED_OPEN) || (OCS_Control[OCS_NUM8].nControl == CONTROL_TIMED_CLOSE) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(OCS_Control[OCS_NUM8].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									OCS_Control[OCS_NUM8].preOPID = OCS_Control[OCS_NUM8].newOPID;
								}
								Debug_printf(" OCS #8 nControl=%d OPID=%d OprTime=%d\n",
										OCS_Control[OCS_NUM8].nControl, OCS_Control[OCS_NUM8].newOPID, OCS_Control[OCS_NUM8].nOprTime);

								Master_Actuator_OCS_Updata_Process(OCS_Control[OCS_NUM8]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" OCS #8 Control error or Keep\n");
						}
						break;
					}
				}
				if(ActNode.bConMode == MULTI_MODE) Master_Write_Multiple_Ack_Process(pPacket);
			}
//======================================================================================================
			else if(ActNode.nRequestAddr >= 599 && ActNode.nRequestAddr < 631) {
				Debug_printf("   ==>> OCS Control B Request!!!\n");
//========================================================================================================
				ActNode.nMultiConCount = (uint16_t)( (pPacket[4] << 8) + pPacket[5]);
				ActNode.nConCount = (uint8_t)(ActNode.nMultiConCount / 4);
				tempAddr = ActNode.nRequestAddr;

				ActNode.nRejCount = (631 - ActNode.nRequestAddr) / 4;		// ===== Max Count Check
				if(ActNode.nConCount > ActNode.nRejCount) return;

				if(ActNode.nConCount == 1) ActNode.bConMode = SINGLE_MODE;
				else if (ActNode.nConCount > 1) ActNode.bConMode = MULTI_MODE;

				for(uint8_t cnt = 0; cnt < ActNode.nConCount; cnt++) {
					ActNode.nRequestAddr = tempAddr;
					tempAddr += 4;
					Debug_printf("ActNode.nRequestAddr = %d : %d\n",cnt, ActNode.nRequestAddr);

//========================================================================================================
					switch(ActNode.nRequestAddr) {

					case CON_OCSWITCH_9_CMD_ADDR :
						if(Actuator.OCSwitch_Num9 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								OCS_Control[OCS_NUM9].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								OCS_Control[OCS_NUM9].newOPID  = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								OCS_Control[OCS_NUM9].nOprTime = rData;
							}
							else{
								OCS_Control[OCS_NUM9].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								OCS_Control[OCS_NUM9].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								OCS_Control[OCS_NUM9].nOprTime = rData;
							}
							OCS_Control[OCS_NUM9].nOCS_Num = OCS_NUM9;

							if( (OCS_Control[OCS_NUM9].preOPID < OCS_Control[OCS_NUM9].newOPID) && ( (OCS_Control[OCS_NUM9].nControl == CONTROL_STOP) ||
								(OCS_Control[OCS_NUM9].nControl == CONTROL_OPEN) || (OCS_Control[OCS_NUM9].nControl == CONTROL_CLOSE) ||
								(OCS_Control[OCS_NUM9].nControl == CONTROL_TIMED_OPEN) || (OCS_Control[OCS_NUM9].nControl == CONTROL_TIMED_CLOSE) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(OCS_Control[OCS_NUM9].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									OCS_Control[OCS_NUM9].preOPID = OCS_Control[OCS_NUM9].newOPID;
								}
								Debug_printf(" OCS #9 nControl=%d OPID=%d OprTime=%d\n",
										OCS_Control[OCS_NUM9].nControl, OCS_Control[OCS_NUM9].newOPID, OCS_Control[OCS_NUM9].nOprTime);

								Master_Actuator_OCS_EXT_Updata_Process(OCS_Control[OCS_NUM9]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" OCS #9 Control error or Keep\n");
						}
						break;
					case CON_OCSWITCH_10_CMD_ADDR :
						if(Actuator.OCSwitch_Num10 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								OCS_Control[OCS_NUM10].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								OCS_Control[OCS_NUM10].newOPID  = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								OCS_Control[OCS_NUM10].nOprTime = rData;
							}
							else{
								OCS_Control[OCS_NUM10].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								OCS_Control[OCS_NUM10].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								OCS_Control[OCS_NUM10].nOprTime = rData;
							}
							OCS_Control[OCS_NUM10].nOCS_Num = OCS_NUM10;

							if( (OCS_Control[OCS_NUM10].preOPID < OCS_Control[OCS_NUM10].newOPID) && ( (OCS_Control[OCS_NUM10].nControl == CONTROL_STOP) ||
								(OCS_Control[OCS_NUM10].nControl == CONTROL_OPEN) || (OCS_Control[OCS_NUM10].nControl == CONTROL_CLOSE) ||
								(OCS_Control[OCS_NUM10].nControl == CONTROL_TIMED_OPEN) || (OCS_Control[OCS_NUM10].nControl == CONTROL_TIMED_CLOSE) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(OCS_Control[OCS_NUM10].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									OCS_Control[OCS_NUM10].preOPID = OCS_Control[OCS_NUM10].newOPID;
								}
								Debug_printf(" OCS #10 nControl=%d OPID=%d OprTime=%d\n",
										OCS_Control[OCS_NUM10].nControl, OCS_Control[OCS_NUM10].newOPID, OCS_Control[OCS_NUM10].nOprTime);

								Master_Actuator_OCS_EXT_Updata_Process(OCS_Control[OCS_NUM10]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" OCS #10 Control error or Keep\n");
						}
						break;
					case CON_OCSWITCH_11_CMD_ADDR :
						if(Actuator.OCSwitch_Num11 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								OCS_Control[OCS_NUM11].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								OCS_Control[OCS_NUM11].newOPID  = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								OCS_Control[OCS_NUM11].nOprTime = rData;
							}
							else{
								OCS_Control[OCS_NUM11].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								OCS_Control[OCS_NUM11].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								OCS_Control[OCS_NUM11].nOprTime = rData;
							}
							OCS_Control[OCS_NUM11].nOCS_Num = OCS_NUM11;

							if( (OCS_Control[OCS_NUM11].preOPID < OCS_Control[OCS_NUM11].newOPID) && ( (OCS_Control[OCS_NUM11].nControl == CONTROL_STOP) ||
								(OCS_Control[OCS_NUM11].nControl == CONTROL_OPEN) || (OCS_Control[OCS_NUM11].nControl == CONTROL_CLOSE) ||
								(OCS_Control[OCS_NUM11].nControl == CONTROL_TIMED_OPEN) || (OCS_Control[OCS_NUM11].nControl == CONTROL_TIMED_CLOSE) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(OCS_Control[OCS_NUM11].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									OCS_Control[OCS_NUM11].preOPID = OCS_Control[OCS_NUM11].newOPID;
								}
								Debug_printf(" OCS #11 nControl=%d OPID=%d OprTime=%d\n",
										OCS_Control[OCS_NUM11].nControl, OCS_Control[OCS_NUM11].newOPID, OCS_Control[OCS_NUM11].nOprTime);

								Master_Actuator_OCS_EXT_Updata_Process(OCS_Control[OCS_NUM11]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" OCS #11 Control error or Keep\n");
						}
						break;
					case CON_OCSWITCH_12_CMD_ADDR :
						if(Actuator.OCSwitch_Num12 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								OCS_Control[OCS_NUM12].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								OCS_Control[OCS_NUM12].newOPID  = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								OCS_Control[OCS_NUM12].nOprTime = rData;
							}
							else{
								OCS_Control[OCS_NUM12].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								OCS_Control[OCS_NUM12].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								OCS_Control[OCS_NUM12].nOprTime = rData;
							}
							OCS_Control[OCS_NUM12].nOCS_Num = OCS_NUM12;

							if( (OCS_Control[OCS_NUM12].preOPID < OCS_Control[OCS_NUM12].newOPID) && ( (OCS_Control[OCS_NUM12].nControl == CONTROL_STOP) ||
								(OCS_Control[OCS_NUM12].nControl == CONTROL_OPEN) || (OCS_Control[OCS_NUM12].nControl == CONTROL_CLOSE) ||
								(OCS_Control[OCS_NUM12].nControl == CONTROL_TIMED_OPEN) || (OCS_Control[OCS_NUM12].nControl == CONTROL_TIMED_CLOSE) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(OCS_Control[OCS_NUM12].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									OCS_Control[OCS_NUM12].preOPID = OCS_Control[OCS_NUM12].newOPID;
								}
								Debug_printf(" OCS #12 nControl=%d OPID=%d OprTime=%d\n",
										OCS_Control[OCS_NUM12].nControl, OCS_Control[OCS_NUM12].newOPID, OCS_Control[OCS_NUM12].nOprTime);

								Master_Actuator_OCS_EXT_Updata_Process(OCS_Control[OCS_NUM12]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" OCS #12 Control error or Keep\n");
						}
						break;
					case CON_OCSWITCH_13_CMD_ADDR :
						if(Actuator.OCSwitch_Num13 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								OCS_Control[OCS_NUM13].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								OCS_Control[OCS_NUM13].newOPID  = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								OCS_Control[OCS_NUM13].nOprTime = rData;
							}
							else{
								OCS_Control[OCS_NUM13].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								OCS_Control[OCS_NUM13].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								OCS_Control[OCS_NUM13].nOprTime = rData;
							}
							OCS_Control[OCS_NUM13].nOCS_Num = OCS_NUM13;

							if( (OCS_Control[OCS_NUM13].preOPID < OCS_Control[OCS_NUM13].newOPID) && ( (OCS_Control[OCS_NUM13].nControl == CONTROL_STOP) ||
								(OCS_Control[OCS_NUM13].nControl == CONTROL_OPEN) || (OCS_Control[OCS_NUM13].nControl == CONTROL_CLOSE) ||
								(OCS_Control[OCS_NUM13].nControl == CONTROL_TIMED_OPEN) || (OCS_Control[OCS_NUM13].nControl == CONTROL_TIMED_CLOSE) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(OCS_Control[OCS_NUM13].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									OCS_Control[OCS_NUM13].preOPID = OCS_Control[OCS_NUM13].newOPID;
								}
								Debug_printf(" OCS #13 nControl=%d OPID=%d OprTime=%d\n",
										OCS_Control[OCS_NUM13].nControl, OCS_Control[OCS_NUM13].newOPID, OCS_Control[OCS_NUM13].nOprTime);

								Master_Actuator_OCS_EXT_Updata_Process(OCS_Control[OCS_NUM13]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" OCS #13 Control error or Keep\n");
						}
						break;
					case CON_OCSWITCH_14_CMD_ADDR :
						if(Actuator.OCSwitch_Num14 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								OCS_Control[OCS_NUM14].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								OCS_Control[OCS_NUM14].newOPID  = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								OCS_Control[OCS_NUM14].nOprTime = rData;
							}
							else{
								OCS_Control[OCS_NUM14].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								OCS_Control[OCS_NUM14].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								OCS_Control[OCS_NUM14].nOprTime = rData;
							}
							OCS_Control[OCS_NUM14].nOCS_Num = OCS_NUM14;

							if( (OCS_Control[OCS_NUM14].preOPID < OCS_Control[OCS_NUM14].newOPID) && ( (OCS_Control[OCS_NUM14].nControl == CONTROL_STOP) ||
								(OCS_Control[OCS_NUM14].nControl == CONTROL_OPEN) || (OCS_Control[OCS_NUM14].nControl == CONTROL_CLOSE) ||
								(OCS_Control[OCS_NUM14].nControl == CONTROL_TIMED_OPEN) || (OCS_Control[OCS_NUM14].nControl == CONTROL_TIMED_CLOSE) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(OCS_Control[OCS_NUM14].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									OCS_Control[OCS_NUM14].preOPID = OCS_Control[OCS_NUM14].newOPID;
								}
								Debug_printf(" OCS #14 nControl=%d OPID=%d OprTime=%d\n",
										OCS_Control[OCS_NUM14].nControl, OCS_Control[OCS_NUM14].newOPID, OCS_Control[OCS_NUM14].nOprTime);

								Master_Actuator_OCS_EXT_Updata_Process(OCS_Control[OCS_NUM14]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" OCS #14 Control error or Keep\n");
						}
						break;
					case CON_OCSWITCH_15_CMD_ADDR :
						if(Actuator.OCSwitch_Num15 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								OCS_Control[OCS_NUM15].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								OCS_Control[OCS_NUM15].newOPID  = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								OCS_Control[OCS_NUM15].nOprTime = rData;
							}
							else{
								OCS_Control[OCS_NUM15].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								OCS_Control[OCS_NUM15].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								OCS_Control[OCS_NUM15].nOprTime = rData;
							}
							OCS_Control[OCS_NUM15].nOCS_Num = OCS_NUM15;

							if( (OCS_Control[OCS_NUM15].preOPID < OCS_Control[OCS_NUM15].newOPID) && ( (OCS_Control[OCS_NUM15].nControl == CONTROL_STOP) ||
								(OCS_Control[OCS_NUM15].nControl == CONTROL_OPEN) || (OCS_Control[OCS_NUM15].nControl == CONTROL_CLOSE) ||
								(OCS_Control[OCS_NUM15].nControl == CONTROL_TIMED_OPEN) || (OCS_Control[OCS_NUM15].nControl == CONTROL_TIMED_CLOSE) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(OCS_Control[OCS_NUM15].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									OCS_Control[OCS_NUM15].preOPID = OCS_Control[OCS_NUM15].newOPID;
								}
								Debug_printf(" OCS #15 nControl=%d OPID=%d OprTime=%d\n",
										OCS_Control[OCS_NUM15].nControl, OCS_Control[OCS_NUM15].newOPID, OCS_Control[OCS_NUM15].nOprTime);

								Master_Actuator_OCS_EXT_Updata_Process(OCS_Control[OCS_NUM15]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" OCS #15 Control error or Keep\n");
						}
						break;
					case CON_OCSWITCH_16_CMD_ADDR :
						if(Actuator.OCSwitch_Num16 == INSTALLED) {
							if(ActNode.bConMode == SINGLE_MODE) {
								OCS_Control[OCS_NUM16].nControl = (uint16_t)( (pPacket[7] << 8) + pPacket[8]);
								OCS_Control[OCS_NUM16].newOPID  = (uint16_t)( (pPacket[9] << 8) + pPacket[10]);
								rData  = pPacket[13] << 24;
								rData |= pPacket[14] << 16;
								rData |= pPacket[11] << 8;
								rData |= pPacket[12];
								OCS_Control[OCS_NUM16].nOprTime = rData;
							}
							else{
								OCS_Control[OCS_NUM16].nControl = (uint16_t)( (pPacket[((8*cnt)+7)+0] << 8) + pPacket[((8*cnt)+7)+1]);
								OCS_Control[OCS_NUM16].newOPID  = (uint16_t)( (pPacket[((8*cnt)+7)+2] << 8) + pPacket[((8*cnt)+7)+3]);
								rData =  pPacket[((8*cnt)+7)+6] << 24;
								rData |= pPacket[((8*cnt)+7)+7] << 16;
								rData |= pPacket[((8*cnt)+7)+4] << 8;
								rData |= pPacket[((8*cnt)+7)+5];
								OCS_Control[OCS_NUM16].nOprTime = rData;
							}
							OCS_Control[OCS_NUM16].nOCS_Num = OCS_NUM16;

							if( (OCS_Control[OCS_NUM16].preOPID < OCS_Control[OCS_NUM16].newOPID) &&( (OCS_Control[OCS_NUM16].nControl == CONTROL_STOP) ||
								(OCS_Control[OCS_NUM16].nControl == CONTROL_OPEN) || (OCS_Control[OCS_NUM16].nControl == CONTROL_CLOSE) ||
								(OCS_Control[OCS_NUM16].nControl == CONTROL_TIMED_OPEN) || (OCS_Control[OCS_NUM16].nControl == CONTROL_TIMED_CLOSE) ) ) {

								// OPID 변경 코드, OPID가 최대치(0xFFFF, 65535)에 도달 했을 경우 모든 채널의 OPID를 0으로 초기화한다.
								if(OCS_Control[OCS_NUM16].newOPID == 0xFFFF)
								{
									Master_OPID_Reset_Process();
								}
								else
								{
									OCS_Control[OCS_NUM16].preOPID = OCS_Control[OCS_NUM16].newOPID;
								}
								Debug_printf(" OCS #16 nControl=%d OPID=%d OprTime=%d\n",
										OCS_Control[OCS_NUM16].nControl, OCS_Control[OCS_NUM16].newOPID, OCS_Control[OCS_NUM16].nOprTime);

								Master_Actuator_OCS_EXT_Updata_Process(OCS_Control[OCS_NUM16]);
								if(ActNode.bConMode == SINGLE_MODE) Master_Write_Multiple_Ack_Process(pPacket);
							}
							else Debug_printf(" OCS #16 Control error or Keep\n");
						}
						break;
					}
				}
				if(ActNode.bConMode == MULTI_MODE) Master_Write_Multiple_Ack_Process(pPacket);
			}
			else {
				Debug_printf(" Max Address Over Error : %02X\r\n", pPacket[1]);
				Master_Exception_0x02_Ack_Process(pPacket[1]);
			}
		}
		else {
			Debug_printf(" Function Error : %02X\r\n", pPacket[1]);
			Master_Exception_0x01_Ack_Process(pPacket[1]);
		}
	}
	Master_Ring_Buffer.nSaveIndex = Master_Ring_Buffer.nProcessIndex = 0;
}

//===========================================================================
void Master_Actuator_OCS_Updata_Process(OCSWITCH_CONTROL Control)
{
	if(Control.nOCS_Num == OCS_NUM1) {
		ActNode.NodeReg[CON_OCSWITCH_1_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_OCSWITCH_1_OPID_17_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_OCSWITCH_1_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[OCSWITCH_1_OPID_17_ADDR] = Control.newOPID;
		ActNode.NodeReg[OCSWITCH_1_STATUS_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_1_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			OCS_Control[OCS_NUM1].OCSCmd = OCS_STOP;
		}
		else if(Control.nControl  == CONTROL_OPEN ) {
			OCS_Control[OCS_NUM1].OCSCmd = OCS_OPEN;
		}
		else if(Control.nControl  == CONTROL_CLOSE ) {
			OCS_Control[OCS_NUM1].OCSCmd = OCS_CLOSE;
		}
		else if(Control.nControl  == CONTROL_TIMED_OPEN ) {
			OCS_Control[OCS_NUM1].OCSCmd = OCS_TIMED_OPEN;
			OCS_Control[OCS_NUM1].nHoldingTime = Control.nOprTime;
		}
		else if(Control.nControl  == CONTROL_TIMED_CLOSE ) {
			OCS_Control[OCS_NUM1].OCSCmd = OCS_TIMED_CLOSE;
			OCS_Control[OCS_NUM1].nHoldingTime = Control.nOprTime;
		}
		Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;

	}
	else if(Control.nOCS_Num == OCS_NUM2) {
		ActNode.NodeReg[CON_OCSWITCH_2_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_OCSWITCH_2_OPID_18_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_OCSWITCH_2_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[OCSWITCH_2_OPID_18_ADDR] = Control.newOPID;
		ActNode.NodeReg[OCSWITCH_2_STATUS_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_2_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			OCS_Control[OCS_NUM2].OCSCmd = OCS_STOP;
		}
		else if(Control.nControl  == CONTROL_OPEN ) {
			OCS_Control[OCS_NUM2].OCSCmd = OCS_OPEN;
		}
		else if(Control.nControl  == CONTROL_CLOSE ) {
			OCS_Control[OCS_NUM2].OCSCmd = OCS_CLOSE;
		}
		else if(Control.nControl  == CONTROL_TIMED_OPEN ) {
			OCS_Control[OCS_NUM2].OCSCmd = OCS_TIMED_OPEN;
			OCS_Control[OCS_NUM2].nHoldingTime = Control.nOprTime;
		}
		else if(Control.nControl  == CONTROL_TIMED_CLOSE ) {
			OCS_Control[OCS_NUM2].OCSCmd = OCS_TIMED_CLOSE;
			OCS_Control[OCS_NUM2].nHoldingTime = Control.nOprTime;
		}
		Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
	}
	else if(Control.nOCS_Num == OCS_NUM3) {
		ActNode.NodeReg[CON_OCSWITCH_3_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_OCSWITCH_3_OPID_19_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_OCSWITCH_3_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[OCSWITCH_3_OPID_19_ADDR] = Control.newOPID;
		ActNode.NodeReg[OCSWITCH_3_STATUS_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_3_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			OCS_Control[OCS_NUM3].OCSCmd = OCS_STOP;
		}
		else if(Control.nControl  == CONTROL_OPEN ) {
			OCS_Control[OCS_NUM3].OCSCmd = OCS_OPEN;
		}
		else if(Control.nControl  == CONTROL_CLOSE ) {
			OCS_Control[OCS_NUM3].OCSCmd = OCS_CLOSE;
		}
		else if(Control.nControl  == CONTROL_TIMED_OPEN ) {
			OCS_Control[OCS_NUM3].OCSCmd = OCS_TIMED_OPEN;
			OCS_Control[OCS_NUM3].nHoldingTime = Control.nOprTime;
		}
		else if(Control.nControl  == CONTROL_TIMED_CLOSE ) {
			OCS_Control[OCS_NUM3].OCSCmd = OCS_TIMED_CLOSE;
			OCS_Control[OCS_NUM3].nHoldingTime = Control.nOprTime;
		}
		Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
	}
	else if(Control.nOCS_Num == OCS_NUM4) {
		ActNode.NodeReg[CON_OCSWITCH_4_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_OCSWITCH_4_OPID_20_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_OCSWITCH_4_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[OCSWITCH_4_OPID_20_ADDR] = Control.newOPID;
		ActNode.NodeReg[OCSWITCH_4_STATUS_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_4_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			OCS_Control[OCS_NUM4].OCSCmd = OCS_STOP;
		}
		else if(Control.nControl  == CONTROL_OPEN ) {
			OCS_Control[OCS_NUM4].OCSCmd = OCS_OPEN;
		}
		else if(Control.nControl  == CONTROL_CLOSE ) {
			OCS_Control[OCS_NUM4].OCSCmd = OCS_CLOSE;
		}
		else if(Control.nControl  == CONTROL_TIMED_OPEN ) {
			OCS_Control[OCS_NUM4].OCSCmd = OCS_TIMED_OPEN;
			OCS_Control[OCS_NUM4].nHoldingTime = Control.nOprTime;
		}
		else if(Control.nControl  == CONTROL_TIMED_CLOSE ) {
			OCS_Control[OCS_NUM4].OCSCmd = OCS_TIMED_CLOSE;
			OCS_Control[OCS_NUM4].nHoldingTime = Control.nOprTime;
		}
		Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
	}
	else if(Control.nOCS_Num == OCS_NUM5) {
		ActNode.NodeReg[CON_OCSWITCH_5_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_OCSWITCH_5_OPID_21_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_OCSWITCH_5_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[OCSWITCH_5_OPID_21_ADDR] = Control.newOPID;
		ActNode.NodeReg[OCSWITCH_5_STATUS_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_5_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			OCS_Control[OCS_NUM5].OCSCmd = OCS_STOP;
		}
		else if(Control.nControl  == CONTROL_OPEN ) {
			OCS_Control[OCS_NUM5].OCSCmd = OCS_OPEN;
		}
		else if(Control.nControl  == CONTROL_CLOSE ) {
			OCS_Control[OCS_NUM5].OCSCmd = OCS_CLOSE;
		}
		else if(Control.nControl  == CONTROL_TIMED_OPEN ) {
			OCS_Control[OCS_NUM5].OCSCmd = OCS_TIMED_OPEN;
			OCS_Control[OCS_NUM5].nHoldingTime = Control.nOprTime;
		}
		else if(Control.nControl  == CONTROL_TIMED_CLOSE ) {
			OCS_Control[OCS_NUM5].OCSCmd = OCS_TIMED_CLOSE;
			OCS_Control[OCS_NUM5].nHoldingTime = Control.nOprTime;
		}
		Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
	}
	else if(Control.nOCS_Num == OCS_NUM6) {
		ActNode.NodeReg[CON_OCSWITCH_6_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_OCSWITCH_6_OPID_22_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_OCSWITCH_6_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[OCSWITCH_6_OPID_22_ADDR] = Control.newOPID;
		ActNode.NodeReg[OCSWITCH_6_STATUS_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_6_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			OCS_Control[OCS_NUM6].OCSCmd = OCS_STOP;
		}
		else if(Control.nControl  == CONTROL_OPEN ) {
			OCS_Control[OCS_NUM6].OCSCmd = OCS_OPEN;
		}
		else if(Control.nControl  == CONTROL_CLOSE ) {
			OCS_Control[OCS_NUM6].OCSCmd = OCS_CLOSE;
		}
		else if(Control.nControl  == CONTROL_TIMED_OPEN ) {
			OCS_Control[OCS_NUM6].OCSCmd = OCS_TIMED_OPEN;
			OCS_Control[OCS_NUM6].nHoldingTime = Control.nOprTime;
		}
		else if(Control.nControl  == CONTROL_TIMED_CLOSE ) {
			OCS_Control[OCS_NUM6].OCSCmd = OCS_TIMED_CLOSE;
			OCS_Control[OCS_NUM6].nHoldingTime = Control.nOprTime;
		}
		Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
	}
	else if(Control.nOCS_Num == OCS_NUM7) {
		ActNode.NodeReg[CON_OCSWITCH_7_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_OCSWITCH_7_OPID_23_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_OCSWITCH_7_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[OCSWITCH_7_OPID_23_ADDR] = Control.newOPID;
		ActNode.NodeReg[OCSWITCH_7_STATUS_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_7_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			OCS_Control[OCS_NUM7].OCSCmd = OCS_STOP;
		}
		else if(Control.nControl  == CONTROL_OPEN ) {
			OCS_Control[OCS_NUM7].OCSCmd = OCS_OPEN;
		}
		else if(Control.nControl  == CONTROL_CLOSE ) {
			OCS_Control[OCS_NUM7].OCSCmd = OCS_CLOSE;
		}
		else if(Control.nControl  == CONTROL_TIMED_OPEN ) {
			OCS_Control[OCS_NUM7].OCSCmd = OCS_TIMED_OPEN;
			OCS_Control[OCS_NUM7].nHoldingTime = Control.nOprTime;
		}
		else if(Control.nControl  == CONTROL_TIMED_CLOSE ) {
			OCS_Control[OCS_NUM7].OCSCmd = OCS_TIMED_CLOSE;
			OCS_Control[OCS_NUM7].nHoldingTime = Control.nOprTime;
		}
		Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
	}
	else if(Control.nOCS_Num == OCS_NUM8) {
		ActNode.NodeReg[CON_OCSWITCH_8_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_OCSWITCH_8_OPID_24_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_OCSWITCH_8_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[OCSWITCH_8_OPID_24_ADDR] = Control.newOPID;
		ActNode.NodeReg[OCSWITCH_8_STATUS_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_8_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			OCS_Control[OCS_NUM8].OCSCmd = OCS_STOP;
		}
		else if(Control.nControl  == CONTROL_OPEN ) {
			OCS_Control[OCS_NUM8].OCSCmd = OCS_OPEN;
		}
		else if(Control.nControl  == CONTROL_CLOSE ) {
			OCS_Control[OCS_NUM8].OCSCmd = OCS_CLOSE;
		}
		else if(Control.nControl  == CONTROL_TIMED_OPEN ) {
			OCS_Control[OCS_NUM8].OCSCmd = OCS_TIMED_OPEN;
			OCS_Control[OCS_NUM8].nHoldingTime = Control.nOprTime;
		}
		else if(Control.nControl  == CONTROL_TIMED_CLOSE ) {
			OCS_Control[OCS_NUM8].OCSCmd = OCS_TIMED_CLOSE;
			OCS_Control[OCS_NUM8].nHoldingTime = Control.nOprTime;
		}
		Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
	}
}
//===========================================================================
void Master_Actuator_OCS_EXT_Updata_Process(OCSWITCH_CONTROL Control)
{
	if(Control.nOCS_Num == OCS_NUM9) {
		ActNode.NodeReg[CON_OCSWITCH_9_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_OCSWITCH_9_OPID_25_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_OCSWITCH_9_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[OCSWITCH_9_OPID_25_ADDR] = Control.newOPID;
		ActNode.NodeReg[OCSWITCH_9_STATUS_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_9_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			OCS_Control[OCS_NUM9].OCSCmd = OCS_STOP;
		}
		else if(Control.nControl  == CONTROL_OPEN ) {
			OCS_Control[OCS_NUM9].OCSCmd = OCS_OPEN;
		}
		else if(Control.nControl  == CONTROL_CLOSE ) {
			OCS_Control[OCS_NUM9].OCSCmd = OCS_CLOSE;
		}
		else if(Control.nControl  == CONTROL_TIMED_OPEN ) {
			OCS_Control[OCS_NUM9].OCSCmd = OCS_TIMED_OPEN;
			OCS_Control[OCS_NUM9].nHoldingTime = Control.nOprTime;
		}
		else if(Control.nControl  == CONTROL_TIMED_CLOSE ) {
			OCS_Control[OCS_NUM9].OCSCmd = OCS_TIMED_CLOSE;
			OCS_Control[OCS_NUM9].nHoldingTime = Control.nOprTime;
		}

		if(NodeVersion == RELAY_16CH_VER)
		{
			Actuator_OCS_EXT_Control.Step = OCS_EXT_RELAY_STEP_WRITE_CMD;
		}
		else if(NodeVersion == RELAY_32CH_VER)
		{
			Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
		}
	}
	else if(Control.nOCS_Num == OCS_NUM10) {
		ActNode.NodeReg[CON_OCSWITCH_10_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_OCSWITCH_10_OPID_26_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_OCSWITCH_10_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[OCSWITCH_10_OPID_26_ADDR] = Control.newOPID;
		ActNode.NodeReg[OCSWITCH_10_STATUS_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_10_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			OCS_Control[OCS_NUM10].OCSCmd = OCS_STOP;
		}
		else if(Control.nControl  == CONTROL_OPEN ) {
			OCS_Control[OCS_NUM10].OCSCmd = OCS_OPEN;
		}
		else if(Control.nControl  == CONTROL_CLOSE ) {
			OCS_Control[OCS_NUM10].OCSCmd = OCS_CLOSE;
		}
		else if(Control.nControl  == CONTROL_TIMED_OPEN ) {
			OCS_Control[OCS_NUM10].OCSCmd = OCS_TIMED_OPEN;
			OCS_Control[OCS_NUM10].nHoldingTime = Control.nOprTime;
		}
		else if(Control.nControl  == CONTROL_TIMED_CLOSE ) {
			OCS_Control[OCS_NUM10].OCSCmd = OCS_TIMED_CLOSE;
			OCS_Control[OCS_NUM10].nHoldingTime = Control.nOprTime;
		}

		if(NodeVersion == RELAY_16CH_VER)
		{
			Actuator_OCS_EXT_Control.Step = OCS_EXT_RELAY_STEP_WRITE_CMD;
		}
		else if(NodeVersion == RELAY_32CH_VER)
		{
			Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
		}
	}
	else if(Control.nOCS_Num == OCS_NUM11) {
		ActNode.NodeReg[CON_OCSWITCH_11_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_OCSWITCH_11_OPID_27_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_OCSWITCH_11_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[OCSWITCH_11_OPID_27_ADDR] = Control.newOPID;
		ActNode.NodeReg[OCSWITCH_11_STATUS_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_11_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			OCS_Control[OCS_NUM11].OCSCmd = OCS_STOP;
		}
		else if(Control.nControl  == CONTROL_OPEN ) {
			OCS_Control[OCS_NUM11].OCSCmd = OCS_OPEN;
		}
		else if(Control.nControl  == CONTROL_CLOSE ) {
			OCS_Control[OCS_NUM11].OCSCmd = OCS_CLOSE;
		}
		else if(Control.nControl  == CONTROL_TIMED_OPEN ) {
			OCS_Control[OCS_NUM11].OCSCmd = OCS_TIMED_OPEN;
			OCS_Control[OCS_NUM11].nHoldingTime = Control.nOprTime;
		}
		else if(Control.nControl  == CONTROL_TIMED_CLOSE ) {
			OCS_Control[OCS_NUM11].OCSCmd = OCS_TIMED_CLOSE;
			OCS_Control[OCS_NUM11].nHoldingTime = Control.nOprTime;
		}

		if(NodeVersion == RELAY_16CH_VER)
		{
			Actuator_OCS_EXT_Control.Step = OCS_EXT_RELAY_STEP_WRITE_CMD;
		}
		else if(NodeVersion == RELAY_32CH_VER)
		{
			Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
		}
	}
	else if(Control.nOCS_Num == OCS_NUM12) {
		ActNode.NodeReg[CON_OCSWITCH_12_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_OCSWITCH_12_OPID_28_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_OCSWITCH_12_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[OCSWITCH_12_OPID_28_ADDR] = Control.newOPID;
		ActNode.NodeReg[OCSWITCH_12_STATUS_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_12_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			OCS_Control[OCS_NUM12].OCSCmd = OCS_STOP;
		}
		else if(Control.nControl  == CONTROL_OPEN ) {
			OCS_Control[OCS_NUM12].OCSCmd = OCS_OPEN;
		}
		else if(Control.nControl  == CONTROL_CLOSE ) {
			OCS_Control[OCS_NUM12].OCSCmd = OCS_CLOSE;
		}
		else if(Control.nControl  == CONTROL_TIMED_OPEN ) {
			OCS_Control[OCS_NUM12].OCSCmd = OCS_TIMED_OPEN;
			OCS_Control[OCS_NUM12].nHoldingTime = Control.nOprTime;
		}
		else if(Control.nControl  == CONTROL_TIMED_CLOSE ) {
			OCS_Control[OCS_NUM12].OCSCmd = OCS_TIMED_CLOSE;
			OCS_Control[OCS_NUM12].nHoldingTime = Control.nOprTime;
		}

		if(NodeVersion == RELAY_16CH_VER)
		{
			Actuator_OCS_EXT_Control.Step = OCS_EXT_RELAY_STEP_WRITE_CMD;
		}
		else if(NodeVersion == RELAY_32CH_VER)
		{
			Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
		}
	}
	else if(Control.nOCS_Num == OCS_NUM13) {
		ActNode.NodeReg[CON_OCSWITCH_13_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_OCSWITCH_13_OPID_29_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_OCSWITCH_13_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[OCSWITCH_13_OPID_29_ADDR] = Control.newOPID;
		ActNode.NodeReg[OCSWITCH_13_STATUS_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_13_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			OCS_Control[OCS_NUM13].OCSCmd = OCS_STOP;
		}
		else if(Control.nControl  == CONTROL_OPEN ) {
			OCS_Control[OCS_NUM13].OCSCmd = OCS_OPEN;
		}
		else if(Control.nControl  == CONTROL_CLOSE ) {
			OCS_Control[OCS_NUM13].OCSCmd = OCS_CLOSE;
		}
		else if(Control.nControl  == CONTROL_TIMED_OPEN ) {
			OCS_Control[OCS_NUM13].OCSCmd = OCS_TIMED_OPEN;
			OCS_Control[OCS_NUM13].nHoldingTime = Control.nOprTime;
		}
		else if(Control.nControl  == CONTROL_TIMED_CLOSE ) {
			OCS_Control[OCS_NUM13].OCSCmd = OCS_TIMED_CLOSE;
			OCS_Control[OCS_NUM13].nHoldingTime = Control.nOprTime;
		}

		if(NodeVersion == RELAY_16CH_VER)
		{
			Actuator_OCS_EXT_Control.Step = OCS_EXT_RELAY_STEP_WRITE_CMD;
		}
		else if(NodeVersion == RELAY_32CH_VER)
		{
			Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
		}
	}
	else if(Control.nOCS_Num == OCS_NUM14) {
		ActNode.NodeReg[CON_OCSWITCH_14_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_OCSWITCH_14_OPID_30_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_OCSWITCH_14_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[OCSWITCH_14_OPID_30_ADDR] = Control.newOPID;
		ActNode.NodeReg[OCSWITCH_14_STATUS_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_14_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			OCS_Control[OCS_NUM14].OCSCmd = OCS_STOP;
		}
		else if(Control.nControl  == CONTROL_OPEN ) {
			OCS_Control[OCS_NUM14].OCSCmd = OCS_OPEN;
		}
		else if(Control.nControl  == CONTROL_CLOSE ) {
			OCS_Control[OCS_NUM14].OCSCmd = OCS_CLOSE;
		}
		else if(Control.nControl  == CONTROL_TIMED_OPEN ) {
			OCS_Control[OCS_NUM14].OCSCmd = OCS_TIMED_OPEN;
			OCS_Control[OCS_NUM14].nHoldingTime = Control.nOprTime;
		}
		else if(Control.nControl  == CONTROL_TIMED_CLOSE ) {
			OCS_Control[OCS_NUM14].OCSCmd = OCS_TIMED_CLOSE;
			OCS_Control[OCS_NUM14].nHoldingTime = Control.nOprTime;
		}

		if(NodeVersion == RELAY_16CH_VER)
		{
			Actuator_OCS_EXT_Control.Step = OCS_EXT_RELAY_STEP_WRITE_CMD;
		}
		else if(NodeVersion == RELAY_32CH_VER)
		{
			Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
		}
	}
	else if(Control.nOCS_Num == OCS_NUM15) {
		ActNode.NodeReg[CON_OCSWITCH_15_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_OCSWITCH_15_OPID_31_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_OCSWITCH_15_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[OCSWITCH_15_OPID_31_ADDR] = Control.newOPID;
		ActNode.NodeReg[OCSWITCH_15_STATUS_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_15_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			OCS_Control[OCS_NUM15].OCSCmd = OCS_STOP;
		}
		else if(Control.nControl  == CONTROL_OPEN ) {
			OCS_Control[OCS_NUM15].OCSCmd = OCS_OPEN;
		}
		else if(Control.nControl  == CONTROL_CLOSE ) {
			OCS_Control[OCS_NUM15].OCSCmd = OCS_CLOSE;
		}
		else if(Control.nControl  == CONTROL_TIMED_OPEN ) {
			OCS_Control[OCS_NUM15].OCSCmd = OCS_TIMED_OPEN;
			OCS_Control[OCS_NUM15].nHoldingTime = Control.nOprTime;
		}
		else if(Control.nControl  == CONTROL_TIMED_CLOSE ) {
			OCS_Control[OCS_NUM15].OCSCmd = OCS_TIMED_CLOSE;
			OCS_Control[OCS_NUM15].nHoldingTime = Control.nOprTime;
		}

		if(NodeVersion == RELAY_16CH_VER)
		{
			Actuator_OCS_EXT_Control.Step = OCS_EXT_RELAY_STEP_WRITE_CMD;
		}
		else if(NodeVersion == RELAY_32CH_VER)
		{
			Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
		}
	}
	else if(Control.nOCS_Num == OCS_NUM16) {
		ActNode.NodeReg[CON_OCSWITCH_16_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_OCSWITCH_16_OPID_32_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_OCSWITCH_16_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[OCSWITCH_16_OPID_32_ADDR] = Control.newOPID;
		ActNode.NodeReg[OCSWITCH_16_STATUS_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_16_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			OCS_Control[OCS_NUM16].OCSCmd = OCS_STOP;
		}
		else if(Control.nControl  == CONTROL_OPEN ) {
			OCS_Control[OCS_NUM16].OCSCmd = OCS_OPEN;
		}
		else if(Control.nControl  == CONTROL_CLOSE ) {
			OCS_Control[OCS_NUM16].OCSCmd = OCS_CLOSE;
		}
		else if(Control.nControl  == CONTROL_TIMED_OPEN ) {
			OCS_Control[OCS_NUM16].OCSCmd = OCS_TIMED_OPEN;
			OCS_Control[OCS_NUM16].nHoldingTime = Control.nOprTime;
		}
		else if(Control.nControl  == CONTROL_TIMED_CLOSE ) {
			OCS_Control[OCS_NUM16].OCSCmd = OCS_TIMED_CLOSE;
			OCS_Control[OCS_NUM16].nHoldingTime = Control.nOprTime;
		}

		if(NodeVersion == RELAY_16CH_VER)
		{
			Actuator_OCS_EXT_Control.Step = OCS_EXT_RELAY_STEP_WRITE_CMD;
		}
		else if(NodeVersion == RELAY_32CH_VER)
		{
			Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
		}
	}
}

void Master_Actuator_OnTime_Process(void)
{
	Debug_printf(">>\n");
//=============================================================================================================
	if(OCS_Control[OCS_NUM1].OCStatus == OCS_TIME_OPEN_START) {
		OCS_Control[OCS_NUM1].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM1].mTMR_1Sec);
		OCS_Control[OCS_NUM1].nRemainingTime = OCS_Control[OCS_NUM1].nHoldingTime - OCS_Control[OCS_NUM1].nGetTime;

		Debug_printf(" Open  1 nRemainingTime : %d\n",OCS_Control[OCS_NUM1].nRemainingTime);
		if(OCS_Control[OCS_NUM1].nRemainingTime <= 0 || (OCS_Control[OCS_NUM1].nHoldingTime < OCS_Control[OCS_NUM1].nGetTime)) {
			OCS_Control[OCS_NUM1].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_1_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_1_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM1].OCSCmd = OCS_STOP;
			Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[OCSWITCH_1_TIME_ADDR] = OCS_Control[OCS_NUM1].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_1_STATUS_ADDR] = COMMON_OPENING;
		}
	}
	else if(OCS_Control[OCS_NUM1].OCStatus == OCS_TIME_CLOSE_START){
		OCS_Control[OCS_NUM1].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM1].mTMR_1Sec);
		OCS_Control[OCS_NUM1].nRemainingTime = OCS_Control[OCS_NUM1].nHoldingTime - OCS_Control[OCS_NUM1].nGetTime;

		Debug_printf(" Close 1 nRemainingTime : %d\n",OCS_Control[OCS_NUM1].nRemainingTime);
		if(OCS_Control[OCS_NUM1].nRemainingTime <= 0 || (OCS_Control[OCS_NUM1].nHoldingTime < OCS_Control[OCS_NUM1].nGetTime)) {
			OCS_Control[OCS_NUM1].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_1_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_1_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM1].OCSCmd = OCS_STOP;
			Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[OCSWITCH_1_TIME_ADDR] = OCS_Control[OCS_NUM1].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_1_STATUS_ADDR] = COMMON_CLOSING;
		}
	}
	else if(OCS_Control[OCS_NUM1].OCStatus == OCS_OPEN_START){
		Debug_printf(" Keep Open  1 \n");

		ActNode.NodeReg[OCSWITCH_1_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_1_STATUS_ADDR] = COMMON_OPENING;
	}
	else if(OCS_Control[OCS_NUM1].OCStatus == OCS_CLOSE_START){
		Debug_printf(" Keep Close 1 \n");

		ActNode.NodeReg[OCSWITCH_1_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_1_STATUS_ADDR] = COMMON_CLOSING;
	}
	else if(OCS_Control[OCS_NUM1].OCStatus == OCS_RUN_STOP){
		ActNode.NodeReg[OCSWITCH_1_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_1_STATUS_ADDR] = COMMON_STOP;
	}
//=======================================================================================================================
	if(OCS_Control[OCS_NUM2].OCStatus == OCS_TIME_OPEN_START) {
		OCS_Control[OCS_NUM2].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM2].mTMR_1Sec);
		OCS_Control[OCS_NUM2].nRemainingTime = OCS_Control[OCS_NUM2].nHoldingTime - OCS_Control[OCS_NUM2].nGetTime;

		Debug_printf(" Open  2 nRemainingTime : %d\n",OCS_Control[OCS_NUM2].nRemainingTime);
		if(OCS_Control[OCS_NUM2].nRemainingTime <= 0 || (OCS_Control[OCS_NUM2].nHoldingTime < OCS_Control[OCS_NUM2].nGetTime)) {
			OCS_Control[OCS_NUM2].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_2_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_2_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM2].OCSCmd = OCS_STOP;
			Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[OCSWITCH_2_TIME_ADDR] = OCS_Control[OCS_NUM2].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_2_STATUS_ADDR] = COMMON_OPENING;
		}
	}
	else if(OCS_Control[OCS_NUM2].OCStatus == OCS_TIME_CLOSE_START){
		OCS_Control[OCS_NUM2].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM2].mTMR_1Sec);
		OCS_Control[OCS_NUM2].nRemainingTime = OCS_Control[OCS_NUM2].nHoldingTime - OCS_Control[OCS_NUM2].nGetTime;

		Debug_printf(" Close 2 nRemainingTime : %d\n",OCS_Control[OCS_NUM2].nRemainingTime);
		if(OCS_Control[OCS_NUM2].nRemainingTime <= 0 || (OCS_Control[OCS_NUM2].nHoldingTime < OCS_Control[OCS_NUM2].nGetTime)) {
			OCS_Control[OCS_NUM2].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_2_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_2_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM2].OCSCmd = OCS_STOP;
			Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[OCSWITCH_2_TIME_ADDR] = OCS_Control[OCS_NUM2].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_2_STATUS_ADDR] = COMMON_CLOSING;
		}
	}
	else if(OCS_Control[OCS_NUM2].OCStatus == OCS_OPEN_START){
		Debug_printf(" Keep Open  2 \n");

		ActNode.NodeReg[OCSWITCH_2_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_2_STATUS_ADDR] = COMMON_OPENING;
	}
	else if(OCS_Control[OCS_NUM2].OCStatus == OCS_CLOSE_START){
		Debug_printf(" Keep Close 2 \n");

		ActNode.NodeReg[OCSWITCH_2_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_2_STATUS_ADDR] = COMMON_CLOSING;
	}
	else if(OCS_Control[OCS_NUM2].OCStatus == OCS_RUN_STOP){
		ActNode.NodeReg[OCSWITCH_2_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_2_STATUS_ADDR] = COMMON_STOP;
	}
//=======================================================================================================================
	if(OCS_Control[OCS_NUM3].OCStatus == OCS_TIME_OPEN_START) {
		OCS_Control[OCS_NUM3].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM3].mTMR_1Sec);
		OCS_Control[OCS_NUM3].nRemainingTime = OCS_Control[OCS_NUM3].nHoldingTime - OCS_Control[OCS_NUM3].nGetTime;

		Debug_printf(" Open  3 nRemainingTime : %d\n",OCS_Control[OCS_NUM3].nRemainingTime);
		if(OCS_Control[OCS_NUM3].nRemainingTime <= 0 || (OCS_Control[OCS_NUM3].nHoldingTime < OCS_Control[OCS_NUM3].nGetTime)) {
			OCS_Control[OCS_NUM3].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_3_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_3_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM3].OCSCmd = OCS_STOP;
			Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[OCSWITCH_3_TIME_ADDR] = OCS_Control[OCS_NUM3].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_3_STATUS_ADDR] = COMMON_OPENING;
		}
	}
	else if(OCS_Control[OCS_NUM3].OCStatus == OCS_TIME_CLOSE_START){
		OCS_Control[OCS_NUM3].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM3].mTMR_1Sec);
		OCS_Control[OCS_NUM3].nRemainingTime = OCS_Control[OCS_NUM3].nHoldingTime - OCS_Control[OCS_NUM3].nGetTime;

		Debug_printf(" Close 3 nRemainingTime : %d\n",OCS_Control[OCS_NUM3].nRemainingTime);
		if(OCS_Control[OCS_NUM3].nRemainingTime <= 0 || (OCS_Control[OCS_NUM3].nHoldingTime < OCS_Control[OCS_NUM3].nGetTime)) {
			OCS_Control[OCS_NUM3].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_3_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_3_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM3].OCSCmd = OCS_STOP;
			Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[OCSWITCH_3_TIME_ADDR] = OCS_Control[OCS_NUM3].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_3_STATUS_ADDR] = COMMON_CLOSING;
		}
	}
	else if(OCS_Control[OCS_NUM3].OCStatus == OCS_OPEN_START){
		Debug_printf(" Keep Open  3 \n");

		ActNode.NodeReg[OCSWITCH_3_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_3_STATUS_ADDR] = COMMON_OPENING;
	}
	else if(OCS_Control[OCS_NUM3].OCStatus == OCS_CLOSE_START){
		Debug_printf(" Keep Close 3 \n");

		ActNode.NodeReg[OCSWITCH_3_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_3_STATUS_ADDR] = COMMON_CLOSING;
	}
	else if(OCS_Control[OCS_NUM3].OCStatus == OCS_RUN_STOP){
		ActNode.NodeReg[OCSWITCH_3_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_3_STATUS_ADDR] = COMMON_STOP;
	}
//=======================================================================================================================
	if(OCS_Control[OCS_NUM4].OCStatus == OCS_TIME_OPEN_START) {
		OCS_Control[OCS_NUM4].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM4].mTMR_1Sec);
		OCS_Control[OCS_NUM4].nRemainingTime = OCS_Control[OCS_NUM4].nHoldingTime - OCS_Control[OCS_NUM4].nGetTime;

		Debug_printf(" Open  4 nRemainingTime : %d\n",OCS_Control[OCS_NUM4].nRemainingTime);
		if(OCS_Control[OCS_NUM4].nRemainingTime <= 0 || (OCS_Control[OCS_NUM4].nHoldingTime < OCS_Control[OCS_NUM4].nGetTime)) {
			OCS_Control[OCS_NUM4].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_4_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_4_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM4].OCSCmd = OCS_STOP;
			Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[OCSWITCH_4_TIME_ADDR] = OCS_Control[OCS_NUM4].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_4_STATUS_ADDR] = COMMON_OPENING;
		}
	}
	else if(OCS_Control[OCS_NUM4].OCStatus == OCS_TIME_CLOSE_START){
		OCS_Control[OCS_NUM4].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM4].mTMR_1Sec);
		OCS_Control[OCS_NUM4].nRemainingTime = OCS_Control[OCS_NUM4].nHoldingTime - OCS_Control[OCS_NUM4].nGetTime;

		Debug_printf(" Close 4 nRemainingTime : %d\n",OCS_Control[OCS_NUM4].nRemainingTime);
		if(OCS_Control[OCS_NUM4].nRemainingTime <= 0 || (OCS_Control[OCS_NUM4].nHoldingTime < OCS_Control[OCS_NUM4].nGetTime)) {
			OCS_Control[OCS_NUM4].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_4_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_4_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM4].OCSCmd = OCS_STOP;
			Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[OCSWITCH_4_TIME_ADDR] = OCS_Control[OCS_NUM4].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_4_STATUS_ADDR] = COMMON_CLOSING;
		}
	}
	else if(OCS_Control[OCS_NUM4].OCStatus == OCS_OPEN_START){
		Debug_printf(" Keep Open  4 \n");

		ActNode.NodeReg[OCSWITCH_4_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_4_STATUS_ADDR] = COMMON_OPENING;
	}
	else if(OCS_Control[OCS_NUM4].OCStatus == OCS_CLOSE_START){
		Debug_printf(" Keep Close 4 \n");

		ActNode.NodeReg[OCSWITCH_4_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_4_STATUS_ADDR] = COMMON_CLOSING;
	}
	else if(OCS_Control[OCS_NUM4].OCStatus == OCS_RUN_STOP){
		ActNode.NodeReg[OCSWITCH_4_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_4_STATUS_ADDR] = COMMON_STOP;
	}
//=======================================================================================================================
	if(OCS_Control[OCS_NUM5].OCStatus == OCS_TIME_OPEN_START) {
		OCS_Control[OCS_NUM5].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM5].mTMR_1Sec);
		OCS_Control[OCS_NUM5].nRemainingTime = OCS_Control[OCS_NUM5].nHoldingTime - OCS_Control[OCS_NUM5].nGetTime;

		Debug_printf(" Open  5 nRemainingTime : %d\n",OCS_Control[OCS_NUM5].nRemainingTime);
		if(OCS_Control[OCS_NUM5].nRemainingTime <= 0|| (OCS_Control[OCS_NUM5].nHoldingTime < OCS_Control[OCS_NUM5].nGetTime)) {
			OCS_Control[OCS_NUM5].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_5_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_5_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM5].OCSCmd = OCS_STOP;
			Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[OCSWITCH_5_TIME_ADDR] = OCS_Control[OCS_NUM5].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_5_STATUS_ADDR] = COMMON_OPENING;
		}
	}
	else if(OCS_Control[OCS_NUM5].OCStatus == OCS_TIME_CLOSE_START){
		OCS_Control[OCS_NUM5].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM5].mTMR_1Sec);
		OCS_Control[OCS_NUM5].nRemainingTime = OCS_Control[OCS_NUM5].nHoldingTime - OCS_Control[OCS_NUM5].nGetTime;

		Debug_printf(" Close 5 nRemainingTime : %d\n",OCS_Control[OCS_NUM5].nRemainingTime);
		if(OCS_Control[OCS_NUM5].nRemainingTime <= 0 || (OCS_Control[OCS_NUM5].nHoldingTime < OCS_Control[OCS_NUM5].nGetTime)) {
			OCS_Control[OCS_NUM5].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_5_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_5_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM5].OCSCmd = OCS_STOP;
			Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[OCSWITCH_5_TIME_ADDR] = OCS_Control[OCS_NUM5].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_5_STATUS_ADDR] = COMMON_CLOSING;
		}
	}
	else if(OCS_Control[OCS_NUM5].OCStatus == OCS_OPEN_START){
		Debug_printf(" Keep Open  5 \n");

		ActNode.NodeReg[OCSWITCH_5_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_5_STATUS_ADDR] = COMMON_OPENING;
	}
	else if(OCS_Control[OCS_NUM5].OCStatus == OCS_CLOSE_START){
		Debug_printf(" Keep Close 5 \n");

		ActNode.NodeReg[OCSWITCH_5_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_5_STATUS_ADDR] = COMMON_CLOSING;
	}
	else if(OCS_Control[OCS_NUM5].OCStatus == OCS_RUN_STOP){
		ActNode.NodeReg[OCSWITCH_5_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_5_STATUS_ADDR] = COMMON_STOP;
	}
//=======================================================================================================================
	if(OCS_Control[OCS_NUM6].OCStatus == OCS_TIME_OPEN_START) {
		OCS_Control[OCS_NUM6].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM6].mTMR_1Sec);
		OCS_Control[OCS_NUM6].nRemainingTime = OCS_Control[OCS_NUM6].nHoldingTime - OCS_Control[OCS_NUM6].nGetTime;

		Debug_printf(" Open  6 nRemainingTime : %d\n",OCS_Control[OCS_NUM6].nRemainingTime);
		if(OCS_Control[OCS_NUM6].nRemainingTime <= 0 || (OCS_Control[OCS_NUM6].nHoldingTime < OCS_Control[OCS_NUM6].nGetTime)) {
			OCS_Control[OCS_NUM6].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_6_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_6_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM6].OCSCmd = OCS_STOP;
			Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[OCSWITCH_6_TIME_ADDR] = OCS_Control[OCS_NUM6].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_6_STATUS_ADDR] = COMMON_OPENING;
		}
	}
	else if(OCS_Control[OCS_NUM6].OCStatus == OCS_TIME_CLOSE_START){
		OCS_Control[OCS_NUM6].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM6].mTMR_1Sec);
		OCS_Control[OCS_NUM6].nRemainingTime = OCS_Control[OCS_NUM6].nHoldingTime - OCS_Control[OCS_NUM6].nGetTime;

		Debug_printf(" Close 6 nRemainingTime : %d\n",OCS_Control[OCS_NUM6].nRemainingTime);
		if(OCS_Control[OCS_NUM6].nRemainingTime <= 0 || (OCS_Control[OCS_NUM6].nHoldingTime < OCS_Control[OCS_NUM6].nGetTime)) {
			OCS_Control[OCS_NUM6].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_6_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_6_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM6].OCSCmd = OCS_STOP;
			Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[OCSWITCH_6_TIME_ADDR] = OCS_Control[OCS_NUM6].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_6_STATUS_ADDR] = COMMON_CLOSING;
		}
	}
	else if(OCS_Control[OCS_NUM6].OCStatus == OCS_OPEN_START){
		Debug_printf(" Keep Open  6 \n");

		ActNode.NodeReg[OCSWITCH_6_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_6_STATUS_ADDR] = COMMON_OPENING;
	}
	else if(OCS_Control[OCS_NUM6].OCStatus == OCS_CLOSE_START){
		Debug_printf(" Keep Close 6 \n");

		ActNode.NodeReg[OCSWITCH_6_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_6_STATUS_ADDR] = COMMON_CLOSING;
	}
	else if(OCS_Control[OCS_NUM6].OCStatus == OCS_RUN_STOP){
		ActNode.NodeReg[OCSWITCH_6_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_6_STATUS_ADDR] = COMMON_STOP;
	}
//=======================================================================================================================
	if(OCS_Control[OCS_NUM7].OCStatus == OCS_TIME_OPEN_START) {
		OCS_Control[OCS_NUM7].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM7].mTMR_1Sec);
		OCS_Control[OCS_NUM7].nRemainingTime = OCS_Control[OCS_NUM7].nHoldingTime - OCS_Control[OCS_NUM7].nGetTime;

		Debug_printf(" Open  7 nRemainingTime : %d\n",OCS_Control[OCS_NUM7].nRemainingTime);
		if(OCS_Control[OCS_NUM7].nRemainingTime <= 0 || (OCS_Control[OCS_NUM7].nHoldingTime < OCS_Control[OCS_NUM7].nGetTime)) {
			OCS_Control[OCS_NUM7].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_7_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_7_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM7].OCSCmd = OCS_STOP;
			Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[OCSWITCH_7_TIME_ADDR] = OCS_Control[OCS_NUM7].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_7_STATUS_ADDR] = COMMON_OPENING;
		}
	}
	else if(OCS_Control[OCS_NUM7].OCStatus == OCS_TIME_CLOSE_START){
		OCS_Control[OCS_NUM7].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM7].mTMR_1Sec);
		OCS_Control[OCS_NUM7].nRemainingTime = OCS_Control[OCS_NUM7].nHoldingTime - OCS_Control[OCS_NUM7].nGetTime;

		Debug_printf(" Close 7 nRemainingTime : %d\n",OCS_Control[OCS_NUM7].nRemainingTime);
		if(OCS_Control[OCS_NUM7].nRemainingTime <= 0 || (OCS_Control[OCS_NUM7].nHoldingTime < OCS_Control[OCS_NUM7].nGetTime) ) {
			OCS_Control[OCS_NUM7].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_7_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_7_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM7].OCSCmd = OCS_STOP;
			Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[OCSWITCH_7_TIME_ADDR] = OCS_Control[OCS_NUM7].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_7_STATUS_ADDR] = COMMON_CLOSING;
		}
	}
	else if(OCS_Control[OCS_NUM7].OCStatus == OCS_OPEN_START){
		Debug_printf(" Keep Open  7 \n");

		ActNode.NodeReg[OCSWITCH_7_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_7_STATUS_ADDR] = COMMON_OPENING;
	}
	else if(OCS_Control[OCS_NUM7].OCStatus == OCS_CLOSE_START){
		Debug_printf(" Keep Close 7 \n");

		ActNode.NodeReg[OCSWITCH_7_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_7_STATUS_ADDR] = COMMON_CLOSING;
	}
	else if(OCS_Control[OCS_NUM7].OCStatus == OCS_RUN_STOP){
		ActNode.NodeReg[OCSWITCH_7_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_7_STATUS_ADDR] = COMMON_STOP;
	}
//=======================================================================================================================
	if(OCS_Control[OCS_NUM8].OCStatus == OCS_TIME_OPEN_START) {
		OCS_Control[OCS_NUM8].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM8].mTMR_1Sec);
		OCS_Control[OCS_NUM8].nRemainingTime = OCS_Control[OCS_NUM8].nHoldingTime - OCS_Control[OCS_NUM8].nGetTime;

		Debug_printf(" Open  8 nRemainingTime : %d\n",OCS_Control[OCS_NUM8].nRemainingTime);
		if(OCS_Control[OCS_NUM8].nRemainingTime <= 0 || (OCS_Control[OCS_NUM8].nHoldingTime < OCS_Control[OCS_NUM8].nGetTime)) {
			OCS_Control[OCS_NUM8].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_8_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_8_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM8].OCSCmd = OCS_STOP;
			Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[OCSWITCH_8_TIME_ADDR] = OCS_Control[OCS_NUM8].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_8_STATUS_ADDR] = COMMON_OPENING;
		}
	}
	else if(OCS_Control[OCS_NUM8].OCStatus == OCS_TIME_CLOSE_START){
		OCS_Control[OCS_NUM8].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM8].mTMR_1Sec);
		OCS_Control[OCS_NUM8].nRemainingTime = OCS_Control[OCS_NUM8].nHoldingTime - OCS_Control[OCS_NUM8].nGetTime;

		Debug_printf(" Close 8 nRemainingTime : %d\n",OCS_Control[OCS_NUM8].nRemainingTime);
		if(OCS_Control[OCS_NUM8].nRemainingTime <= 0 || (OCS_Control[OCS_NUM8].nHoldingTime < OCS_Control[OCS_NUM8].nGetTime)) {
			OCS_Control[OCS_NUM8].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_8_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_8_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM8].OCSCmd = OCS_STOP;
			Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[OCSWITCH_8_TIME_ADDR] = OCS_Control[OCS_NUM8].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_8_STATUS_ADDR] = COMMON_CLOSING;
		}
	}
	else if(OCS_Control[OCS_NUM8].OCStatus == OCS_OPEN_START){
		Debug_printf(" Keep Open  8 \n");

		ActNode.NodeReg[OCSWITCH_8_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_8_STATUS_ADDR] = COMMON_OPENING;
	}
	else if(OCS_Control[OCS_NUM8].OCStatus == OCS_CLOSE_START){
		Debug_printf(" Keep Close 8 \n");

		ActNode.NodeReg[OCSWITCH_8_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_8_STATUS_ADDR] = COMMON_CLOSING;
	}
	else if(OCS_Control[OCS_NUM8].OCStatus == OCS_RUN_STOP){
		ActNode.NodeReg[OCSWITCH_8_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_8_STATUS_ADDR] = COMMON_STOP;
	}
//=================================================================================================================
	if(OCS_Control[OCS_NUM9].OCStatus == OCS_TIME_OPEN_START) {
		OCS_Control[OCS_NUM9].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM9].mTMR_1Sec);
		OCS_Control[OCS_NUM9].nRemainingTime = OCS_Control[OCS_NUM9].nHoldingTime - OCS_Control[OCS_NUM9].nGetTime;

		Debug_printf(" Open  9 nRemainingTime : %d\n",OCS_Control[OCS_NUM9].nRemainingTime);
		if(OCS_Control[OCS_NUM9].nRemainingTime <= 0 || (OCS_Control[OCS_NUM9].nHoldingTime < OCS_Control[OCS_NUM9].nGetTime)) {
			OCS_Control[OCS_NUM9].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_9_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_9_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM9].OCSCmd = OCS_STOP;
			if(NodeVersion == RELAY_16CH_VER)
			{
				Actuator_OCS_EXT_Control.Step = OCS_EXT_RELAY_STEP_WRITE_CMD;
			}
			else if(NodeVersion == RELAY_32CH_VER)
			{
				Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
			}
		}
		else {
			ActNode.NodeReg[OCSWITCH_9_TIME_ADDR] = OCS_Control[OCS_NUM9].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_9_STATUS_ADDR] = COMMON_OPENING;
		}
	}
	else if(OCS_Control[OCS_NUM9].OCStatus == OCS_TIME_CLOSE_START){
		OCS_Control[OCS_NUM9].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM9].mTMR_1Sec);
		OCS_Control[OCS_NUM9].nRemainingTime = OCS_Control[OCS_NUM9].nHoldingTime - OCS_Control[OCS_NUM9].nGetTime;

		Debug_printf(" Close 9 nRemainingTime : %d\n",OCS_Control[OCS_NUM9].nRemainingTime);
		if(OCS_Control[OCS_NUM9].nRemainingTime <= 0 || (OCS_Control[OCS_NUM9].nHoldingTime < OCS_Control[OCS_NUM9].nGetTime)) {
			OCS_Control[OCS_NUM9].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_9_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_9_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM9].OCSCmd = OCS_STOP;
			if(NodeVersion == RELAY_16CH_VER)
			{
				Actuator_OCS_EXT_Control.Step = OCS_EXT_RELAY_STEP_WRITE_CMD;
			}
			else if(NodeVersion == RELAY_32CH_VER)
			{
				Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
			}
		}
		else {
			ActNode.NodeReg[OCSWITCH_9_TIME_ADDR] = OCS_Control[OCS_NUM9].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_9_STATUS_ADDR] = COMMON_CLOSING;
		}
	}
	else if(OCS_Control[OCS_NUM9].OCStatus == OCS_OPEN_START){
		Debug_printf(" Keep Open  9 \n");

		ActNode.NodeReg[OCSWITCH_9_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_9_STATUS_ADDR] = COMMON_OPENING;
	}
	else if(OCS_Control[OCS_NUM9].OCStatus == OCS_CLOSE_START){
		Debug_printf(" Keep Close 9 \n");

		ActNode.NodeReg[OCSWITCH_9_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_9_STATUS_ADDR] = COMMON_CLOSING;
	}
	else if(OCS_Control[OCS_NUM9].OCStatus == OCS_RUN_STOP){
		ActNode.NodeReg[OCSWITCH_9_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_9_STATUS_ADDR] = COMMON_STOP;
	}
//=================================================================================================================
	if(OCS_Control[OCS_NUM10].OCStatus == OCS_TIME_OPEN_START) {
		OCS_Control[OCS_NUM10].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM10].mTMR_1Sec);
		OCS_Control[OCS_NUM10].nRemainingTime = OCS_Control[OCS_NUM10].nHoldingTime - OCS_Control[OCS_NUM10].nGetTime;

		Debug_printf(" Open  10 nRemainingTime : %d\n",OCS_Control[OCS_NUM10].nRemainingTime);
		if(OCS_Control[OCS_NUM10].nRemainingTime <= 0 || (OCS_Control[OCS_NUM10].nHoldingTime < OCS_Control[OCS_NUM10].nGetTime)) {
			OCS_Control[OCS_NUM10].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_10_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_10_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM10].OCSCmd = OCS_STOP;
			if(NodeVersion == RELAY_16CH_VER)
			{
				Actuator_OCS_EXT_Control.Step = OCS_EXT_RELAY_STEP_WRITE_CMD;
			}
			else if(NodeVersion == RELAY_32CH_VER)
			{
				Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
			}
		}
		else {
			ActNode.NodeReg[OCSWITCH_10_TIME_ADDR] = OCS_Control[OCS_NUM10].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_10_STATUS_ADDR] = COMMON_OPENING;
		}
	}
	else if(OCS_Control[OCS_NUM10].OCStatus == OCS_TIME_CLOSE_START){
		OCS_Control[OCS_NUM10].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM10].mTMR_1Sec);
		OCS_Control[OCS_NUM10].nRemainingTime = OCS_Control[OCS_NUM10].nHoldingTime - OCS_Control[OCS_NUM10].nGetTime;

		Debug_printf(" Close 10 nRemainingTime : %d\n",OCS_Control[OCS_NUM10].nRemainingTime);
		if(OCS_Control[OCS_NUM10].nRemainingTime <= 0 || (OCS_Control[OCS_NUM10].nHoldingTime < OCS_Control[OCS_NUM10].nGetTime)) {
			OCS_Control[OCS_NUM10].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_10_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_10_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM10].OCSCmd = OCS_STOP;
			if(NodeVersion == RELAY_16CH_VER)
			{
				Actuator_OCS_EXT_Control.Step = OCS_EXT_RELAY_STEP_WRITE_CMD;
			}
			else if(NodeVersion == RELAY_32CH_VER)
			{
				Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
			}
		}
		else {
			ActNode.NodeReg[OCSWITCH_10_TIME_ADDR] = OCS_Control[OCS_NUM10].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_10_STATUS_ADDR] = COMMON_CLOSING;
		}
	}
	else if(OCS_Control[OCS_NUM10].OCStatus == OCS_OPEN_START){
		Debug_printf(" Keep Open  10 \n");

		ActNode.NodeReg[OCSWITCH_10_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_10_STATUS_ADDR] = COMMON_OPENING;
	}
	else if(OCS_Control[OCS_NUM10].OCStatus == OCS_CLOSE_START){
		Debug_printf(" Keep Close 10 \n");

		ActNode.NodeReg[OCSWITCH_10_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_10_STATUS_ADDR] = COMMON_CLOSING;
	}
	else if(OCS_Control[OCS_NUM10].OCStatus == OCS_RUN_STOP){
		ActNode.NodeReg[OCSWITCH_10_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_10_STATUS_ADDR] = COMMON_STOP;
	}
//=================================================================================================================
	if(OCS_Control[OCS_NUM11].OCStatus == OCS_TIME_OPEN_START) {
		OCS_Control[OCS_NUM11].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM11].mTMR_1Sec);
		OCS_Control[OCS_NUM11].nRemainingTime = OCS_Control[OCS_NUM11].nHoldingTime - OCS_Control[OCS_NUM11].nGetTime;

		Debug_printf(" Open  11 nRemainingTime : %d\n",OCS_Control[OCS_NUM11].nRemainingTime);
		if(OCS_Control[OCS_NUM11].nRemainingTime <= 0 || (OCS_Control[OCS_NUM11].nHoldingTime < OCS_Control[OCS_NUM11].nGetTime)) {
			OCS_Control[OCS_NUM11].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_11_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_11_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM11].OCSCmd = OCS_STOP;
			if(NodeVersion == RELAY_16CH_VER)
			{
				Actuator_OCS_EXT_Control.Step = OCS_EXT_RELAY_STEP_WRITE_CMD;
			}
			else if(NodeVersion == RELAY_32CH_VER)
			{
				Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
			}
		}
		else {
			ActNode.NodeReg[OCSWITCH_11_TIME_ADDR] = OCS_Control[OCS_NUM11].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_11_STATUS_ADDR] = COMMON_OPENING;
		}
	}
	else if(OCS_Control[OCS_NUM11].OCStatus == OCS_TIME_CLOSE_START){
		OCS_Control[OCS_NUM11].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM11].mTMR_1Sec);
		OCS_Control[OCS_NUM11].nRemainingTime = OCS_Control[OCS_NUM11].nHoldingTime - OCS_Control[OCS_NUM11].nGetTime;

		Debug_printf(" Close 11 nRemainingTime : %d\n",OCS_Control[OCS_NUM11].nRemainingTime);
		if(OCS_Control[OCS_NUM11].nRemainingTime <= 0 || (OCS_Control[OCS_NUM11].nHoldingTime < OCS_Control[OCS_NUM11].nGetTime)) {
			OCS_Control[OCS_NUM11].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_11_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_11_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM11].OCSCmd = OCS_STOP;
			if(NodeVersion == RELAY_16CH_VER)
			{
				Actuator_OCS_EXT_Control.Step = OCS_EXT_RELAY_STEP_WRITE_CMD;
			}
			else if(NodeVersion == RELAY_32CH_VER)
			{
				Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
			}
		}
		else {
			ActNode.NodeReg[OCSWITCH_11_TIME_ADDR] = OCS_Control[OCS_NUM11].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_11_STATUS_ADDR] = COMMON_CLOSING;
		}
	}
	else if(OCS_Control[OCS_NUM11].OCStatus == OCS_OPEN_START){
		Debug_printf(" Keep Open  11 \n");

		ActNode.NodeReg[OCSWITCH_11_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_11_STATUS_ADDR] = COMMON_OPENING;
	}
	else if(OCS_Control[OCS_NUM11].OCStatus == OCS_CLOSE_START){
		Debug_printf(" Keep Close 11 \n");

		ActNode.NodeReg[OCSWITCH_11_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_11_STATUS_ADDR] = COMMON_CLOSING;
	}
	else if(OCS_Control[OCS_NUM11].OCStatus == OCS_RUN_STOP){
		ActNode.NodeReg[OCSWITCH_11_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_11_STATUS_ADDR] = COMMON_STOP;
	}
//=================================================================================================================
	if(OCS_Control[OCS_NUM12].OCStatus == OCS_TIME_OPEN_START) {
		OCS_Control[OCS_NUM12].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM12].mTMR_1Sec);
		OCS_Control[OCS_NUM12].nRemainingTime = OCS_Control[OCS_NUM12].nHoldingTime - OCS_Control[OCS_NUM12].nGetTime;

		Debug_printf(" Open  12 nRemainingTime : %d\n",OCS_Control[OCS_NUM12].nRemainingTime);
		if(OCS_Control[OCS_NUM12].nRemainingTime <= 0 || (OCS_Control[OCS_NUM12].nHoldingTime < OCS_Control[OCS_NUM12].nGetTime)) {
			OCS_Control[OCS_NUM12].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_12_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_12_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM12].OCSCmd = OCS_STOP;
			if(NodeVersion == RELAY_16CH_VER)
			{
				Actuator_OCS_EXT_Control.Step = OCS_EXT_RELAY_STEP_WRITE_CMD;
			}
			else if(NodeVersion == RELAY_32CH_VER)
			{
				Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
			}
		}
		else {
			ActNode.NodeReg[OCSWITCH_12_TIME_ADDR] = OCS_Control[OCS_NUM12].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_12_STATUS_ADDR] = COMMON_OPENING;
		}
	}
	else if(OCS_Control[OCS_NUM12].OCStatus == OCS_TIME_CLOSE_START){
		OCS_Control[OCS_NUM12].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM12].mTMR_1Sec);
		OCS_Control[OCS_NUM12].nRemainingTime = OCS_Control[OCS_NUM12].nHoldingTime - OCS_Control[OCS_NUM12].nGetTime;

		Debug_printf(" Close 12 nRemainingTime : %d\n",OCS_Control[OCS_NUM12].nRemainingTime);
		if(OCS_Control[OCS_NUM12].nRemainingTime <= 0 || (OCS_Control[OCS_NUM12].nHoldingTime < OCS_Control[OCS_NUM12].nGetTime)) {
			OCS_Control[OCS_NUM12].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_12_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_12_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM12].OCSCmd = OCS_STOP;
			if(NodeVersion == RELAY_16CH_VER)
			{
				Actuator_OCS_EXT_Control.Step = OCS_EXT_RELAY_STEP_WRITE_CMD;
			}
			else if(NodeVersion == RELAY_32CH_VER)
			{
				Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
			}
		}
		else {
			ActNode.NodeReg[OCSWITCH_12_TIME_ADDR] = OCS_Control[OCS_NUM12].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_12_STATUS_ADDR] = COMMON_CLOSING;
		}
	}
	else if(OCS_Control[OCS_NUM12].OCStatus == OCS_OPEN_START){
		Debug_printf(" Keep Open  12 \n");

		ActNode.NodeReg[OCSWITCH_12_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_12_STATUS_ADDR] = COMMON_OPENING;
	}
	else if(OCS_Control[OCS_NUM12].OCStatus == OCS_CLOSE_START){
		Debug_printf(" Keep Close 12 \n");

		ActNode.NodeReg[OCSWITCH_12_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_12_STATUS_ADDR] = COMMON_CLOSING;
	}
	else if(OCS_Control[OCS_NUM12].OCStatus == OCS_RUN_STOP){
		ActNode.NodeReg[OCSWITCH_12_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_12_STATUS_ADDR] = COMMON_STOP;
	}
//=================================================================================================================
	if(OCS_Control[OCS_NUM13].OCStatus == OCS_TIME_OPEN_START) {
		OCS_Control[OCS_NUM13].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM13].mTMR_1Sec);
		OCS_Control[OCS_NUM13].nRemainingTime = OCS_Control[OCS_NUM13].nHoldingTime - OCS_Control[OCS_NUM13].nGetTime;

		Debug_printf(" Open  13 nRemainingTime : %d\n",OCS_Control[OCS_NUM13].nRemainingTime);
		if(OCS_Control[OCS_NUM13].nRemainingTime <= 0 || (OCS_Control[OCS_NUM13].nHoldingTime < OCS_Control[OCS_NUM13].nGetTime)) {
			OCS_Control[OCS_NUM13].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_13_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_13_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM13].OCSCmd = OCS_STOP;
			if(NodeVersion == RELAY_16CH_VER)
			{
				Actuator_OCS_EXT_Control.Step = OCS_EXT_RELAY_STEP_WRITE_CMD;
			}
			else if(NodeVersion == RELAY_32CH_VER)
			{
				Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
			}
		}
		else {
			ActNode.NodeReg[OCSWITCH_13_TIME_ADDR] = OCS_Control[OCS_NUM13].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_13_STATUS_ADDR] = COMMON_OPENING;
		}
	}
	else if(OCS_Control[OCS_NUM13].OCStatus == OCS_TIME_CLOSE_START){
		OCS_Control[OCS_NUM13].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM13].mTMR_1Sec);
		OCS_Control[OCS_NUM13].nRemainingTime = OCS_Control[OCS_NUM13].nHoldingTime - OCS_Control[OCS_NUM13].nGetTime;

		Debug_printf(" Close 13 nRemainingTime : %d\n",OCS_Control[OCS_NUM13].nRemainingTime);
		if(OCS_Control[OCS_NUM13].nRemainingTime <= 0 || (OCS_Control[OCS_NUM13].nHoldingTime < OCS_Control[OCS_NUM13].nGetTime)) {
			OCS_Control[OCS_NUM13].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_13_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_13_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM13].OCSCmd = OCS_STOP;
			if(NodeVersion == RELAY_16CH_VER)
			{
				Actuator_OCS_EXT_Control.Step = OCS_EXT_RELAY_STEP_WRITE_CMD;
			}
			else if(NodeVersion == RELAY_32CH_VER)
			{
				Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
			}
		}
		else {
			ActNode.NodeReg[OCSWITCH_13_TIME_ADDR] = OCS_Control[OCS_NUM13].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_13_STATUS_ADDR] = COMMON_CLOSING;
		}
	}
	else if(OCS_Control[OCS_NUM13].OCStatus == OCS_OPEN_START){
		Debug_printf(" Keep Open  13 \n");

		ActNode.NodeReg[OCSWITCH_13_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_13_STATUS_ADDR] = COMMON_OPENING;
	}
	else if(OCS_Control[OCS_NUM13].OCStatus == OCS_CLOSE_START){
		Debug_printf(" Keep Close 13 \n");

		ActNode.NodeReg[OCSWITCH_13_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_13_STATUS_ADDR] = COMMON_CLOSING;
	}
	else if(OCS_Control[OCS_NUM13].OCStatus == OCS_RUN_STOP){
		ActNode.NodeReg[OCSWITCH_13_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_13_STATUS_ADDR] = COMMON_STOP;
	}
//=================================================================================================================
	if(OCS_Control[OCS_NUM14].OCStatus == OCS_TIME_OPEN_START) {
		OCS_Control[OCS_NUM14].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM14].mTMR_1Sec);
		OCS_Control[OCS_NUM14].nRemainingTime = OCS_Control[OCS_NUM14].nHoldingTime - OCS_Control[OCS_NUM14].nGetTime;

		Debug_printf(" Open  14 nRemainingTime : %d\n",OCS_Control[OCS_NUM14].nRemainingTime);
		if(OCS_Control[OCS_NUM14].nRemainingTime <= 0 || (OCS_Control[OCS_NUM14].nHoldingTime < OCS_Control[OCS_NUM14].nGetTime)) {
			OCS_Control[OCS_NUM14].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_14_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_14_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM14].OCSCmd = OCS_STOP;
			if(NodeVersion == RELAY_16CH_VER)
			{
				Actuator_OCS_EXT_Control.Step = OCS_EXT_RELAY_STEP_WRITE_CMD;
			}
			else if(NodeVersion == RELAY_32CH_VER)
			{
				Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
			}
		}
		else {
			ActNode.NodeReg[OCSWITCH_14_TIME_ADDR] = OCS_Control[OCS_NUM14].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_14_STATUS_ADDR] = COMMON_OPENING;
		}
	}
	else if(OCS_Control[OCS_NUM14].OCStatus == OCS_TIME_CLOSE_START){
		OCS_Control[OCS_NUM14].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM14].mTMR_1Sec);
		OCS_Control[OCS_NUM14].nRemainingTime = OCS_Control[OCS_NUM14].nHoldingTime - OCS_Control[OCS_NUM14].nGetTime;

		Debug_printf(" Close 14 nRemainingTime : %d\n",OCS_Control[OCS_NUM14].nRemainingTime);
		if(OCS_Control[OCS_NUM14].nRemainingTime <= 0 || (OCS_Control[OCS_NUM14].nHoldingTime < OCS_Control[OCS_NUM14].nGetTime)) {
			OCS_Control[OCS_NUM14].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_14_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_14_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM14].OCSCmd = OCS_STOP;
			if(NodeVersion == RELAY_16CH_VER)
			{
				Actuator_OCS_EXT_Control.Step = OCS_EXT_RELAY_STEP_WRITE_CMD;
			}
			else if(NodeVersion == RELAY_32CH_VER)
			{
				Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
			}
		}
		else {
			ActNode.NodeReg[OCSWITCH_14_TIME_ADDR] = OCS_Control[OCS_NUM14].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_14_STATUS_ADDR] = COMMON_CLOSING;
		}
	}
	else if(OCS_Control[OCS_NUM14].OCStatus == OCS_OPEN_START){
		Debug_printf(" Keep Open  14 \n");

		ActNode.NodeReg[OCSWITCH_14_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_14_STATUS_ADDR] = COMMON_OPENING;
	}
	else if(OCS_Control[OCS_NUM14].OCStatus == OCS_CLOSE_START){
		Debug_printf(" Keep Close 14 \n");

		ActNode.NodeReg[OCSWITCH_14_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_14_STATUS_ADDR] = COMMON_CLOSING;
	}
	else if(OCS_Control[OCS_NUM14].OCStatus == OCS_RUN_STOP){
		ActNode.NodeReg[OCSWITCH_14_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_14_STATUS_ADDR] = COMMON_STOP;
	}
//=================================================================================================================
	if(OCS_Control[OCS_NUM15].OCStatus == OCS_TIME_OPEN_START) {
		OCS_Control[OCS_NUM15].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM15].mTMR_1Sec);
		OCS_Control[OCS_NUM15].nRemainingTime = OCS_Control[OCS_NUM15].nHoldingTime - OCS_Control[OCS_NUM15].nGetTime;

		Debug_printf(" Open  15 nRemainingTime : %d\n",OCS_Control[OCS_NUM15].nRemainingTime);
		if(OCS_Control[OCS_NUM15].nRemainingTime <= 0 || (OCS_Control[OCS_NUM15].nHoldingTime < OCS_Control[OCS_NUM15].nGetTime)) {
			OCS_Control[OCS_NUM15].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_15_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_15_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM15].OCSCmd = OCS_STOP;
			if(NodeVersion == RELAY_16CH_VER)
			{
				Actuator_OCS_EXT_Control.Step = OCS_EXT_RELAY_STEP_WRITE_CMD;
			}
			else if(NodeVersion == RELAY_32CH_VER)
			{
				Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
			}
		}
		else {
			ActNode.NodeReg[OCSWITCH_15_TIME_ADDR] = OCS_Control[OCS_NUM15].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_15_STATUS_ADDR] = COMMON_OPENING;
		}
	}
	else if(OCS_Control[OCS_NUM15].OCStatus == OCS_TIME_CLOSE_START){
		OCS_Control[OCS_NUM15].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM15].mTMR_1Sec);
		OCS_Control[OCS_NUM15].nRemainingTime = OCS_Control[OCS_NUM15].nHoldingTime - OCS_Control[OCS_NUM15].nGetTime;

		Debug_printf(" Close 15 nRemainingTime : %d\n",OCS_Control[OCS_NUM15].nRemainingTime);
		if(OCS_Control[OCS_NUM15].nRemainingTime <= 0 || (OCS_Control[OCS_NUM15].nHoldingTime < OCS_Control[OCS_NUM15].nGetTime)) {
			OCS_Control[OCS_NUM15].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_15_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_15_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM15].OCSCmd = OCS_STOP;
			if(NodeVersion == RELAY_16CH_VER)
			{
				Actuator_OCS_EXT_Control.Step = OCS_EXT_RELAY_STEP_WRITE_CMD;
			}
			else if(NodeVersion == RELAY_32CH_VER)
			{
				Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
			}
		}
		else {
			ActNode.NodeReg[OCSWITCH_15_TIME_ADDR] = OCS_Control[OCS_NUM15].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_15_STATUS_ADDR] = COMMON_CLOSING;
		}
	}
	else if(OCS_Control[OCS_NUM15].OCStatus == OCS_OPEN_START){
		Debug_printf(" Keep Open  15 \n");

		ActNode.NodeReg[OCSWITCH_15_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_15_STATUS_ADDR] = COMMON_OPENING;
	}
	else if(OCS_Control[OCS_NUM15].OCStatus == OCS_CLOSE_START){
		Debug_printf(" Keep Close 15 \n");

		ActNode.NodeReg[OCSWITCH_15_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_15_STATUS_ADDR] = COMMON_CLOSING;
	}
	else if(OCS_Control[OCS_NUM15].OCStatus == OCS_RUN_STOP){
		ActNode.NodeReg[OCSWITCH_15_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_15_STATUS_ADDR] = COMMON_STOP;
	}
//=================================================================================================================
	if(OCS_Control[OCS_NUM16].OCStatus == OCS_TIME_OPEN_START) {
		OCS_Control[OCS_NUM16].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM16].mTMR_1Sec);
		OCS_Control[OCS_NUM16].nRemainingTime = OCS_Control[OCS_NUM16].nHoldingTime - OCS_Control[OCS_NUM16].nGetTime;

		Debug_printf(" Open  16 nRemainingTime : %d\n",OCS_Control[OCS_NUM16].nRemainingTime);
		if(OCS_Control[OCS_NUM16].nRemainingTime <= 0 || (OCS_Control[OCS_NUM16].nHoldingTime < OCS_Control[OCS_NUM16].nGetTime)) {
			OCS_Control[OCS_NUM16].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_16_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_16_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM16].OCSCmd = OCS_STOP;
			if(NodeVersion == RELAY_16CH_VER)
			{
				Actuator_OCS_EXT_Control.Step = OCS_EXT_RELAY_STEP_WRITE_CMD;
			}
			else if(NodeVersion == RELAY_32CH_VER)
			{
				Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
			}
		}
		else {
			ActNode.NodeReg[OCSWITCH_16_TIME_ADDR] = OCS_Control[OCS_NUM16].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_16_STATUS_ADDR] = COMMON_OPENING;
		}
	}
	else if(OCS_Control[OCS_NUM16].OCStatus == OCS_TIME_CLOSE_START){
		OCS_Control[OCS_NUM16].nGetTime = sec_timer_get_sec(OCS_Control[OCS_NUM16].mTMR_1Sec);
		OCS_Control[OCS_NUM16].nRemainingTime = OCS_Control[OCS_NUM16].nHoldingTime - OCS_Control[OCS_NUM16].nGetTime;

		Debug_printf(" Close 16 nRemainingTime : %d\n",OCS_Control[OCS_NUM16].nRemainingTime);
		if(OCS_Control[OCS_NUM16].nRemainingTime <= 0 || (OCS_Control[OCS_NUM16].nHoldingTime < OCS_Control[OCS_NUM16].nGetTime)) {
			OCS_Control[OCS_NUM16].OCStatus = OCS_TIME_STOP;

			ActNode.NodeReg[OCSWITCH_16_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[OCSWITCH_16_TIME_ADDR] = 0;

			OCS_Control[OCS_NUM16].OCSCmd = OCS_STOP;
			if(NodeVersion == RELAY_16CH_VER)
			{
				Actuator_OCS_EXT_Control.Step = OCS_EXT_RELAY_STEP_WRITE_CMD;
			}
			else if(NodeVersion == RELAY_32CH_VER)
			{
				Actuator_OCS_Control.Step = OCS_RELAY_STEP_WRITE_CMD;
			}
		}
		else {
			ActNode.NodeReg[OCSWITCH_16_TIME_ADDR] = OCS_Control[OCS_NUM16].nRemainingTime;
			ActNode.NodeReg[OCSWITCH_16_STATUS_ADDR] = COMMON_CLOSING;
		}
	}
	else if(OCS_Control[OCS_NUM16].OCStatus == OCS_OPEN_START){
		Debug_printf(" Keep Open  16 \n");

		ActNode.NodeReg[OCSWITCH_16_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_16_STATUS_ADDR] = COMMON_OPENING;
	}
	else if(OCS_Control[OCS_NUM16].OCStatus == OCS_CLOSE_START){
		Debug_printf(" Keep Close 16 \n");

		ActNode.NodeReg[OCSWITCH_16_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_16_STATUS_ADDR] = COMMON_CLOSING;
	}
	else if(OCS_Control[OCS_NUM16].OCStatus == OCS_RUN_STOP){
		ActNode.NodeReg[OCSWITCH_16_TIME_ADDR] = 0;
		ActNode.NodeReg[OCSWITCH_16_STATUS_ADDR] = COMMON_STOP;
	}
//=================================================================================================================
//=================================================================================================================
	if(SW_Control[SW_NUM1].SWStatus == SW_TIME_ON_START) {
		SW_Control[SW_NUM1].nGetTime = sec_timer_get_sec(SW_Control[SW_NUM1].mTMR_1Sec);
		SW_Control[SW_NUM1].nRemainingTime = SW_Control[SW_NUM1].nHoldingTime - SW_Control[SW_NUM1].nGetTime;

		Debug_printf(" SW    1 nRemainingTime : %d\n",SW_Control[SW_NUM1].nRemainingTime);
		if(SW_Control[SW_NUM1].nRemainingTime <= 0 || (SW_Control[SW_NUM1].nHoldingTime < SW_Control[SW_NUM1].nGetTime)) {
			SW_Control[SW_NUM1].SWStatus = SW_TIME_STOP;

			ActNode.NodeReg[SWITCH_1_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[SWITCH_1_TIME_ADDR] = 0;

			SW_Control[SW_NUM1].SWCmd = SW_STOP;
			Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[SWITCH_1_TIME_ADDR] = SW_Control[SW_NUM1].nRemainingTime;
			ActNode.NodeReg[SWITCH_1_STATUS_ADDR] = COMMON_ON;
		}
	}
	else if(SW_Control[SW_NUM1].SWStatus == SW_ON_START) {
		Debug_printf(" Keep ON 1 \n");

		ActNode.NodeReg[SWITCH_1_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_1_STATUS_ADDR] = COMMON_ON;
	}
	else if(SW_Control[SW_NUM1].SWStatus == SW_OFF) {
		ActNode.NodeReg[SWITCH_1_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_1_STATUS_ADDR] = COMMON_STOP;
	}
//=======================================================================================================================
	if(SW_Control[SW_NUM2].SWStatus == SW_TIME_ON_START) {
		SW_Control[SW_NUM2].nGetTime = sec_timer_get_sec(SW_Control[SW_NUM2].mTMR_1Sec);
		SW_Control[SW_NUM2].nRemainingTime = SW_Control[SW_NUM2].nHoldingTime - SW_Control[SW_NUM2].nGetTime;

		Debug_printf(" SW    2 nRemainingTime : %d\n",SW_Control[SW_NUM2].nRemainingTime);
		if(SW_Control[SW_NUM2].nRemainingTime <= 0 || (SW_Control[SW_NUM2].nHoldingTime < SW_Control[SW_NUM2].nGetTime)) {
			SW_Control[SW_NUM2].SWStatus = SW_TIME_STOP;

			ActNode.NodeReg[SWITCH_2_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[SWITCH_2_TIME_ADDR] = 0;

			SW_Control[SW_NUM2].SWCmd = SW_STOP;
			Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[SWITCH_2_TIME_ADDR] = SW_Control[SW_NUM2].nRemainingTime;
			ActNode.NodeReg[SWITCH_2_STATUS_ADDR] = COMMON_ON;
		}
	}
	else if(SW_Control[SW_NUM2].SWStatus == SW_ON_START) {
		Debug_printf(" Keep ON 2 \n");

		ActNode.NodeReg[SWITCH_2_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_2_STATUS_ADDR] = COMMON_ON;
	}
	else if(SW_Control[SW_NUM2].SWStatus == SW_OFF) {
		ActNode.NodeReg[SWITCH_2_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_2_STATUS_ADDR] = COMMON_STOP;
	}
//=======================================================================================================================
	if(SW_Control[SW_NUM3].SWStatus == SW_TIME_ON_START) {
		SW_Control[SW_NUM3].nGetTime = sec_timer_get_sec(SW_Control[SW_NUM3].mTMR_1Sec);
		SW_Control[SW_NUM3].nRemainingTime = SW_Control[SW_NUM3].nHoldingTime - SW_Control[SW_NUM3].nGetTime;

		Debug_printf(" SW    3 nRemainingTime : %d\n",SW_Control[SW_NUM3].nRemainingTime);
		if(SW_Control[SW_NUM3].nRemainingTime <= 0 || (SW_Control[SW_NUM3].nHoldingTime < SW_Control[SW_NUM3].nGetTime)) {
			SW_Control[SW_NUM3].SWStatus = SW_TIME_STOP;

			ActNode.NodeReg[SWITCH_3_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[SWITCH_3_TIME_ADDR] = 0;

			SW_Control[SW_NUM3].SWCmd = SW_STOP;
			Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[SWITCH_3_TIME_ADDR] = SW_Control[SW_NUM3].nRemainingTime;
			ActNode.NodeReg[SWITCH_3_STATUS_ADDR] = COMMON_ON;
		}
	}
	else if(SW_Control[SW_NUM3].SWStatus == SW_ON_START) {
		Debug_printf(" Keep ON 3 \n");

		ActNode.NodeReg[SWITCH_3_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_3_STATUS_ADDR] = COMMON_ON;
	}
	else if(SW_Control[SW_NUM3].SWStatus == SW_OFF) {
		ActNode.NodeReg[SWITCH_3_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_3_STATUS_ADDR] = COMMON_STOP;
	}
//=======================================================================================================================
	if(SW_Control[SW_NUM4].SWStatus == SW_TIME_ON_START) {
		SW_Control[SW_NUM4].nGetTime = sec_timer_get_sec(SW_Control[SW_NUM4].mTMR_1Sec);
		SW_Control[SW_NUM4].nRemainingTime = SW_Control[SW_NUM4].nHoldingTime - SW_Control[SW_NUM4].nGetTime;

		Debug_printf(" SW    4 nRemainingTime : %d\n",SW_Control[SW_NUM4].nRemainingTime);
		if(SW_Control[SW_NUM4].nRemainingTime <= 0 || (SW_Control[SW_NUM4].nHoldingTime < SW_Control[SW_NUM4].nGetTime)) {
			SW_Control[SW_NUM4].SWStatus = SW_TIME_STOP;

			ActNode.NodeReg[SWITCH_4_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[SWITCH_4_TIME_ADDR] = 0;

			SW_Control[SW_NUM4].SWCmd = SW_STOP;
			Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[SWITCH_4_TIME_ADDR] = SW_Control[SW_NUM4].nRemainingTime;
			ActNode.NodeReg[SWITCH_4_STATUS_ADDR] = COMMON_ON;
		}
	}
	else if(SW_Control[SW_NUM4].SWStatus == SW_ON_START) {
		Debug_printf(" Keep ON 4 \n");

		ActNode.NodeReg[SWITCH_4_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_4_STATUS_ADDR] = COMMON_ON;
	}
	else if(SW_Control[SW_NUM4].SWStatus == SW_OFF) {
		ActNode.NodeReg[SWITCH_4_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_4_STATUS_ADDR] = COMMON_STOP;
	}
//=======================================================================================================================
	if(SW_Control[SW_NUM5].SWStatus == SW_TIME_ON_START) {
		SW_Control[SW_NUM5].nGetTime = sec_timer_get_sec(SW_Control[SW_NUM5].mTMR_1Sec);
		SW_Control[SW_NUM5].nRemainingTime = SW_Control[SW_NUM5].nHoldingTime - SW_Control[SW_NUM5].nGetTime;

		Debug_printf(" SW    5 nRemainingTime : %d\n",SW_Control[SW_NUM5].nRemainingTime);
		if(SW_Control[SW_NUM5].nRemainingTime <= 0 || (SW_Control[SW_NUM5].nHoldingTime < SW_Control[SW_NUM5].nGetTime)) {
			SW_Control[SW_NUM5].SWStatus = SW_TIME_STOP;

			ActNode.NodeReg[SWITCH_5_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[SWITCH_5_TIME_ADDR] = 0;

			SW_Control[SW_NUM5].SWCmd = SW_STOP;
			Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[SWITCH_5_TIME_ADDR] = SW_Control[SW_NUM5].nRemainingTime;
			ActNode.NodeReg[SWITCH_5_STATUS_ADDR] = COMMON_ON;
		}
	}
	else if(SW_Control[SW_NUM5].SWStatus == SW_ON_START) {
		Debug_printf(" Keep ON 5 \n");

		ActNode.NodeReg[SWITCH_5_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_5_STATUS_ADDR] = COMMON_ON;
	}
	else if(SW_Control[SW_NUM5].SWStatus == SW_OFF) {
		ActNode.NodeReg[SWITCH_5_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_5_STATUS_ADDR] = COMMON_STOP;
	}
//=======================================================================================================================
	if(SW_Control[SW_NUM6].SWStatus == SW_TIME_ON_START) {
		SW_Control[SW_NUM6].nGetTime = sec_timer_get_sec(SW_Control[SW_NUM6].mTMR_1Sec);
		SW_Control[SW_NUM6].nRemainingTime = SW_Control[SW_NUM6].nHoldingTime - SW_Control[SW_NUM6].nGetTime;

		Debug_printf(" SW    6 nRemainingTime : %d\n",SW_Control[SW_NUM6].nRemainingTime);
		if(SW_Control[SW_NUM6].nRemainingTime <= 0 || (SW_Control[SW_NUM6].nHoldingTime < SW_Control[SW_NUM6].nGetTime)) {
			SW_Control[SW_NUM6].SWStatus = SW_TIME_STOP;

			ActNode.NodeReg[SWITCH_6_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[SWITCH_6_TIME_ADDR] = 0;

			SW_Control[SW_NUM6].SWCmd = SW_STOP;
			Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[SWITCH_6_TIME_ADDR] = SW_Control[SW_NUM6].nRemainingTime;
			ActNode.NodeReg[SWITCH_6_STATUS_ADDR] = COMMON_ON;
		}
	}
	else if(SW_Control[SW_NUM6].SWStatus == SW_ON_START) {
		Debug_printf(" Keep ON 6 \n");

		ActNode.NodeReg[SWITCH_6_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_6_STATUS_ADDR] = COMMON_ON;
	}
	else if(SW_Control[SW_NUM6].SWStatus == SW_OFF) {
		ActNode.NodeReg[SWITCH_6_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_6_STATUS_ADDR] = COMMON_STOP;
	}
//=======================================================================================================================
	if(SW_Control[SW_NUM7].SWStatus == SW_TIME_ON_START) {
		SW_Control[SW_NUM7].nGetTime = sec_timer_get_sec(SW_Control[SW_NUM7].mTMR_1Sec);
		SW_Control[SW_NUM7].nRemainingTime = SW_Control[SW_NUM7].nHoldingTime - SW_Control[SW_NUM7].nGetTime;

		Debug_printf(" SW    7 nRemainingTime : %d\n",SW_Control[SW_NUM7].nRemainingTime);
		if(SW_Control[SW_NUM7].nRemainingTime <= 0 || (SW_Control[SW_NUM7].nHoldingTime < SW_Control[SW_NUM7].nGetTime)) {
			SW_Control[SW_NUM7].SWStatus = SW_TIME_STOP;

			ActNode.NodeReg[SWITCH_7_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[SWITCH_7_TIME_ADDR] = 0;

			SW_Control[SW_NUM7].SWCmd = SW_STOP;
			Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[SWITCH_7_TIME_ADDR] = SW_Control[SW_NUM7].nRemainingTime;
			ActNode.NodeReg[SWITCH_7_STATUS_ADDR] = COMMON_ON;
		}
	}
	else if(SW_Control[SW_NUM7].SWStatus == SW_ON_START) {
		Debug_printf(" Keep ON 7 \n");

		ActNode.NodeReg[SWITCH_7_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_7_STATUS_ADDR] = COMMON_ON;
	}
	else if(SW_Control[SW_NUM7].SWStatus == SW_OFF) {
		ActNode.NodeReg[SWITCH_7_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_7_STATUS_ADDR] = COMMON_STOP;
	}
//=======================================================================================================================
	if(SW_Control[SW_NUM8].SWStatus == SW_TIME_ON_START) {
		SW_Control[SW_NUM8].nGetTime = sec_timer_get_sec(SW_Control[SW_NUM8].mTMR_1Sec);
		SW_Control[SW_NUM8].nRemainingTime = SW_Control[SW_NUM8].nHoldingTime - SW_Control[SW_NUM8].nGetTime;

		Debug_printf(" SW    8 nRemainingTime : %d\n",SW_Control[SW_NUM8].nRemainingTime);
		if(SW_Control[SW_NUM8].nRemainingTime <= 0 || (SW_Control[SW_NUM8].nHoldingTime < SW_Control[SW_NUM8].nGetTime)) {
			SW_Control[SW_NUM8].SWStatus = SW_TIME_STOP;

			ActNode.NodeReg[SWITCH_8_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[SWITCH_8_TIME_ADDR] = 0;

			SW_Control[SW_NUM8].SWCmd = SW_STOP;
			Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[SWITCH_8_TIME_ADDR] = SW_Control[SW_NUM8].nRemainingTime;
			ActNode.NodeReg[SWITCH_8_STATUS_ADDR] = COMMON_ON;
		}
	}
	else if(SW_Control[SW_NUM8].SWStatus == SW_ON_START) {
		Debug_printf(" Keep ON 8 \n");

		ActNode.NodeReg[SWITCH_8_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_8_STATUS_ADDR] = COMMON_ON;
	}
	else if(SW_Control[SW_NUM8].SWStatus == SW_OFF) {
		ActNode.NodeReg[SWITCH_8_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_8_STATUS_ADDR] = COMMON_STOP;
	}
//=======================================================================================================================
	if(SW_Control[SW_NUM9].SWStatus == SW_TIME_ON_START) {
		SW_Control[SW_NUM9].nGetTime = sec_timer_get_sec(SW_Control[SW_NUM9].mTMR_1Sec);
		SW_Control[SW_NUM9].nRemainingTime = SW_Control[SW_NUM9].nHoldingTime - SW_Control[SW_NUM9].nGetTime;

		Debug_printf(" SW    9 nRemainingTime : %d\n",SW_Control[SW_NUM9].nRemainingTime);
		if(SW_Control[SW_NUM9].nRemainingTime <= 0 || (SW_Control[SW_NUM9].nHoldingTime < SW_Control[SW_NUM9].nGetTime)) {
			SW_Control[SW_NUM9].SWStatus = SW_TIME_STOP;

			ActNode.NodeReg[SWITCH_9_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[SWITCH_9_TIME_ADDR] = 0;

			SW_Control[SW_NUM9].SWCmd = SW_STOP;
			Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[SWITCH_9_TIME_ADDR] = SW_Control[SW_NUM9].nRemainingTime;
			ActNode.NodeReg[SWITCH_9_STATUS_ADDR] = COMMON_ON;
		}
	}
	else if(SW_Control[SW_NUM9].SWStatus == SW_ON_START) {
		Debug_printf(" Keep ON 9 \n");

		ActNode.NodeReg[SWITCH_9_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_9_STATUS_ADDR] = COMMON_ON;
	}
	else if(SW_Control[SW_NUM9].SWStatus == SW_OFF) {
		ActNode.NodeReg[SWITCH_9_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_9_STATUS_ADDR] = COMMON_STOP;
	}
//=======================================================================================================================
	if(SW_Control[SW_NUM10].SWStatus == SW_TIME_ON_START) {
		SW_Control[SW_NUM10].nGetTime = sec_timer_get_sec(SW_Control[SW_NUM10].mTMR_1Sec);
		SW_Control[SW_NUM10].nRemainingTime = SW_Control[SW_NUM10].nHoldingTime - SW_Control[SW_NUM10].nGetTime;

		Debug_printf(" SW   10 nRemainingTime : %d\n",SW_Control[SW_NUM10].nRemainingTime);
		if(SW_Control[SW_NUM10].nRemainingTime <= 0 || (SW_Control[SW_NUM10].nHoldingTime < SW_Control[SW_NUM10].nGetTime)) {
			SW_Control[SW_NUM10].SWStatus = SW_TIME_STOP;

			ActNode.NodeReg[SWITCH_10_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[SWITCH_10_TIME_ADDR] = 0;

			SW_Control[SW_NUM10].SWCmd = SW_STOP;
			Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[SWITCH_10_TIME_ADDR] = SW_Control[SW_NUM10].nRemainingTime;
			ActNode.NodeReg[SWITCH_10_STATUS_ADDR] = COMMON_ON;
		}
	}
	else if(SW_Control[SW_NUM10].SWStatus == SW_ON_START) {
		Debug_printf(" Keep ON 10 \n");

		ActNode.NodeReg[SWITCH_10_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_10_STATUS_ADDR] = COMMON_ON;
	}
	else if(SW_Control[SW_NUM10].SWStatus == SW_OFF) {
		ActNode.NodeReg[SWITCH_10_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_10_STATUS_ADDR] = COMMON_STOP;
	}
//=======================================================================================================================
	if(SW_Control[SW_NUM11].SWStatus == SW_TIME_ON_START) {
		SW_Control[SW_NUM11].nGetTime = sec_timer_get_sec(SW_Control[SW_NUM11].mTMR_1Sec);
		SW_Control[SW_NUM11].nRemainingTime = SW_Control[SW_NUM11].nHoldingTime - SW_Control[SW_NUM11].nGetTime;

		Debug_printf(" SW   11 nRemainingTime : %d\n",SW_Control[SW_NUM11].nRemainingTime);
		if(SW_Control[SW_NUM11].nRemainingTime <= 0 || (SW_Control[SW_NUM11].nHoldingTime < SW_Control[SW_NUM11].nGetTime)) {
			SW_Control[SW_NUM11].SWStatus = SW_TIME_STOP;

			ActNode.NodeReg[SWITCH_11_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[SWITCH_11_TIME_ADDR] = 0;

			SW_Control[SW_NUM11].SWCmd = SW_STOP;
			Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[SWITCH_11_TIME_ADDR] = SW_Control[SW_NUM11].nRemainingTime;
			ActNode.NodeReg[SWITCH_11_STATUS_ADDR] = COMMON_ON;
		}
	}
	else if(SW_Control[SW_NUM11].SWStatus == SW_ON_START) {
		Debug_printf(" Keep ON 11 \n");

		ActNode.NodeReg[SWITCH_11_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_11_STATUS_ADDR] = COMMON_ON;
	}
	else if(SW_Control[SW_NUM11].SWStatus == SW_OFF) {
		ActNode.NodeReg[SWITCH_11_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_11_STATUS_ADDR] = COMMON_STOP;
	}
//=======================================================================================================================
	if(SW_Control[SW_NUM12].SWStatus == SW_TIME_ON_START) {
		SW_Control[SW_NUM12].nGetTime = sec_timer_get_sec(SW_Control[SW_NUM12].mTMR_1Sec);
		SW_Control[SW_NUM12].nRemainingTime = SW_Control[SW_NUM12].nHoldingTime - SW_Control[SW_NUM12].nGetTime;

		Debug_printf(" SW   12 nRemainingTime : %d\n",SW_Control[SW_NUM12].nRemainingTime);
		if(SW_Control[SW_NUM12].nRemainingTime <= 0 || (SW_Control[SW_NUM12].nHoldingTime < SW_Control[SW_NUM12].nGetTime)) {
			SW_Control[SW_NUM12].SWStatus = SW_TIME_STOP;

			ActNode.NodeReg[SWITCH_12_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[SWITCH_12_TIME_ADDR] = 0;

			SW_Control[SW_NUM12].SWCmd = SW_STOP;
			Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[SWITCH_12_TIME_ADDR] = SW_Control[SW_NUM12].nRemainingTime;
			ActNode.NodeReg[SWITCH_12_STATUS_ADDR] = COMMON_ON;
		}
	}
	else if(SW_Control[SW_NUM12].SWStatus == SW_ON_START) {
		Debug_printf(" Keep ON 12 \n");

		ActNode.NodeReg[SWITCH_12_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_12_STATUS_ADDR] = COMMON_ON;
	}
	else if(SW_Control[SW_NUM12].SWStatus == SW_OFF) {
		ActNode.NodeReg[SWITCH_12_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_12_STATUS_ADDR] = COMMON_STOP;
	}
//=======================================================================================================================
	if(SW_Control[SW_NUM13].SWStatus == SW_TIME_ON_START) {
		SW_Control[SW_NUM13].nGetTime = sec_timer_get_sec(SW_Control[SW_NUM13].mTMR_1Sec);
		SW_Control[SW_NUM13].nRemainingTime = SW_Control[SW_NUM13].nHoldingTime - SW_Control[SW_NUM13].nGetTime;

		Debug_printf(" SW   13 nRemainingTime : %d\n",SW_Control[SW_NUM13].nRemainingTime);
		if(SW_Control[SW_NUM13].nRemainingTime <= 0 || (SW_Control[SW_NUM13].nHoldingTime < SW_Control[SW_NUM13].nGetTime)) {
			SW_Control[SW_NUM13].SWStatus = SW_TIME_STOP;

			ActNode.NodeReg[SWITCH_13_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[SWITCH_13_TIME_ADDR] = 0;

			SW_Control[SW_NUM13].SWCmd = SW_STOP;
			Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[SWITCH_13_TIME_ADDR] = SW_Control[SW_NUM13].nRemainingTime;
			ActNode.NodeReg[SWITCH_13_STATUS_ADDR] = COMMON_ON;
		}
	}
	else if(SW_Control[SW_NUM13].SWStatus == SW_ON_START) {
		Debug_printf(" Keep ON 13 \n");

		ActNode.NodeReg[SWITCH_13_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_13_STATUS_ADDR] = COMMON_ON;
	}
	else if(SW_Control[SW_NUM13].SWStatus == SW_OFF) {
		ActNode.NodeReg[SWITCH_13_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_13_STATUS_ADDR] = COMMON_STOP;
	}
//=======================================================================================================================
	if(SW_Control[SW_NUM14].SWStatus == SW_TIME_ON_START) {
		SW_Control[SW_NUM14].nGetTime = sec_timer_get_sec(SW_Control[SW_NUM14].mTMR_1Sec);
		SW_Control[SW_NUM14].nRemainingTime = SW_Control[SW_NUM14].nHoldingTime - SW_Control[SW_NUM14].nGetTime;

		Debug_printf(" SW   14 nRemainingTime : %d\n",SW_Control[SW_NUM14].nRemainingTime);
		if(SW_Control[SW_NUM14].nRemainingTime <= 0 || (SW_Control[SW_NUM14].nHoldingTime < SW_Control[SW_NUM14].nGetTime)) {
			SW_Control[SW_NUM14].SWStatus = SW_TIME_STOP;

			ActNode.NodeReg[SWITCH_14_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[SWITCH_14_TIME_ADDR] = 0;

			SW_Control[SW_NUM14].SWCmd = SW_STOP;
			Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[SWITCH_14_TIME_ADDR] = SW_Control[SW_NUM14].nRemainingTime;
			ActNode.NodeReg[SWITCH_14_STATUS_ADDR] = COMMON_ON;
		}
	}
	else if(SW_Control[SW_NUM14].SWStatus == SW_ON_START) {
		Debug_printf(" Keep ON 14 \n");

		ActNode.NodeReg[SWITCH_14_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_14_STATUS_ADDR] = COMMON_ON;
	}
	else if(SW_Control[SW_NUM14].SWStatus == SW_OFF) {
		ActNode.NodeReg[SWITCH_14_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_14_STATUS_ADDR] = COMMON_STOP;
	}
//=======================================================================================================================
	if(SW_Control[SW_NUM15].SWStatus == SW_TIME_ON_START) {
		SW_Control[SW_NUM15].nGetTime = sec_timer_get_sec(SW_Control[SW_NUM15].mTMR_1Sec);
		SW_Control[SW_NUM15].nRemainingTime = SW_Control[SW_NUM15].nHoldingTime - SW_Control[SW_NUM15].nGetTime;

		Debug_printf(" SW   15 nRemainingTime : %d\n",SW_Control[SW_NUM15].nRemainingTime);
		if(SW_Control[SW_NUM15].nRemainingTime <= 0 || (SW_Control[SW_NUM15].nHoldingTime < SW_Control[SW_NUM15].nGetTime)) {
			SW_Control[SW_NUM15].SWStatus = SW_TIME_STOP;

			ActNode.NodeReg[SWITCH_15_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[SWITCH_15_TIME_ADDR] = 0;

			SW_Control[SW_NUM15].SWCmd = SW_STOP;
			Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[SWITCH_15_TIME_ADDR] = SW_Control[SW_NUM15].nRemainingTime;
			ActNode.NodeReg[SWITCH_15_STATUS_ADDR] = COMMON_ON;
		}
	}
	else if(SW_Control[SW_NUM15].SWStatus == SW_ON_START) {
		Debug_printf(" Keep ON 15 \n");

		ActNode.NodeReg[SWITCH_15_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_15_STATUS_ADDR] = COMMON_ON;
	}
	else if(SW_Control[SW_NUM15].SWStatus == SW_OFF) {
		ActNode.NodeReg[SWITCH_15_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_15_STATUS_ADDR] = COMMON_STOP;
	}
//=======================================================================================================================
	if(SW_Control[SW_NUM16].SWStatus == SW_TIME_ON_START) {
		SW_Control[SW_NUM16].nGetTime = sec_timer_get_sec(SW_Control[SW_NUM16].mTMR_1Sec);
		SW_Control[SW_NUM16].nRemainingTime = SW_Control[SW_NUM16].nHoldingTime - SW_Control[SW_NUM16].nGetTime;

		Debug_printf(" SW   16 nRemainingTime : %d\n",SW_Control[SW_NUM16].nRemainingTime);
		if(SW_Control[SW_NUM16].nRemainingTime <= 0 || (SW_Control[SW_NUM16].nHoldingTime < SW_Control[SW_NUM16].nGetTime)) {
			SW_Control[SW_NUM16].SWStatus = SW_TIME_STOP;

			ActNode.NodeReg[SWITCH_16_STATUS_ADDR] = COMMON_STOP;
			ActNode.NodeReg[SWITCH_16_TIME_ADDR] = 0;

			SW_Control[SW_NUM16].SWCmd = SW_STOP;
			Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
		}
		else {
			ActNode.NodeReg[SWITCH_16_TIME_ADDR] = SW_Control[SW_NUM16].nRemainingTime;
			ActNode.NodeReg[SWITCH_16_STATUS_ADDR] = COMMON_ON;
		}
	}
	else if(SW_Control[SW_NUM16].SWStatus == SW_ON_START) {
		Debug_printf(" Keep ON 16 \n");

		ActNode.NodeReg[SWITCH_16_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_16_STATUS_ADDR] = COMMON_ON;
	}
	else if(SW_Control[SW_NUM16].SWStatus == SW_OFF) {
		ActNode.NodeReg[SWITCH_16_TIME_ADDR] = 0;
		ActNode.NodeReg[SWITCH_16_STATUS_ADDR] = COMMON_STOP;
	}
}

void Master_Actuator_SW_Updata_Process(SWITCH_CONTROL Control)
{
	if(Control.nSW_Num == SW_NUM1) {
		ActNode.NodeReg[CON_SWITCH_1_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_SWITCH_1_OPID_1_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_SWITCH_1_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[SWITCH_1_OPID_1_ADDR] = Control.newOPID;
		ActNode.NodeReg[SWITCH_1_STATUS_ADDR] = 0;
		ActNode.NodeReg[SWITCH_1_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			SW_Control[SW_NUM1].SWCmd = SW_STOP;
		}
		else if(Control.nControl == CONTROL_ON ) {
			SW_Control[SW_NUM1].SWCmd = SW_ON;
		}
		else if(Control.nControl == CONTROL_TIMED_ON ) {
			SW_Control[SW_NUM1].SWCmd = SW_TIMED_ON;
			SW_Control[SW_NUM1].nHoldingTime = Control.nOprTime;
		}
		Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
	}
	else if(Control.nSW_Num == SW_NUM2) {
		ActNode.NodeReg[CON_SWITCH_2_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_SWITCH_2_OPID_2_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_SWITCH_2_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[SWITCH_2_OPID_2_ADDR] = Control.newOPID;
		ActNode.NodeReg[SWITCH_2_STATUS_ADDR] = 0;
		ActNode.NodeReg[SWITCH_2_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			SW_Control[SW_NUM2].SWCmd = SW_STOP;
		}
		else if(Control.nControl == CONTROL_ON ) {
			SW_Control[SW_NUM2].SWCmd = SW_ON;
		}
		else if(Control.nControl == CONTROL_TIMED_ON ) {
			SW_Control[SW_NUM2].SWCmd = SW_TIMED_ON;
			SW_Control[SW_NUM2].nHoldingTime = Control.nOprTime;
		}
		Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
	}
	else if(Control.nSW_Num == SW_NUM3) {
		ActNode.NodeReg[CON_SWITCH_3_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_SWITCH_3_OPID_3_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_SWITCH_3_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[SWITCH_3_OPID_3_ADDR] = Control.newOPID;
		ActNode.NodeReg[SWITCH_3_STATUS_ADDR] = 0;
		ActNode.NodeReg[SWITCH_3_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			SW_Control[SW_NUM3].SWCmd = SW_STOP;
		}
		else if(Control.nControl == CONTROL_ON ) {
			SW_Control[SW_NUM3].SWCmd = SW_ON;
		}
		else if(Control.nControl == CONTROL_TIMED_ON ) {
			SW_Control[SW_NUM3].SWCmd = SW_TIMED_ON;
			SW_Control[SW_NUM3].nHoldingTime = Control.nOprTime;
		}
		Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
	}
	else if(Control.nSW_Num == SW_NUM4) {
		ActNode.NodeReg[CON_SWITCH_4_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_SWITCH_4_OPID_4_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_SWITCH_4_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[SWITCH_4_OPID_4_ADDR] = Control.newOPID;
		ActNode.NodeReg[SWITCH_4_STATUS_ADDR] = 0;
		ActNode.NodeReg[SWITCH_4_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			SW_Control[SW_NUM4].SWCmd = SW_STOP;
		}
		else if(Control.nControl == CONTROL_ON ) {
			SW_Control[SW_NUM4].SWCmd = SW_ON;
		}
		else if(Control.nControl == CONTROL_TIMED_ON ) {
			SW_Control[SW_NUM4].SWCmd = SW_TIMED_ON;
			SW_Control[SW_NUM4].nHoldingTime = Control.nOprTime;
		}
		Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
	}
	else if(Control.nSW_Num == SW_NUM5) {
		ActNode.NodeReg[CON_SWITCH_5_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_SWITCH_5_OPID_5_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_SWITCH_5_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[SWITCH_5_OPID_5_ADDR] = Control.newOPID;
		ActNode.NodeReg[SWITCH_5_STATUS_ADDR] = 0;
		ActNode.NodeReg[SWITCH_5_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			SW_Control[SW_NUM5].SWCmd = SW_STOP;
		}
		else if(Control.nControl == CONTROL_ON ) {
			SW_Control[SW_NUM5].SWCmd = SW_ON;
		}
		else if(Control.nControl == CONTROL_TIMED_ON ) {
			SW_Control[SW_NUM5].SWCmd = SW_TIMED_ON;
			SW_Control[SW_NUM5].nHoldingTime = Control.nOprTime;
		}
		Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
	}
	else if(Control.nSW_Num == SW_NUM6) {
		ActNode.NodeReg[CON_SWITCH_6_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_SWITCH_6_OPID_6_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_SWITCH_6_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[SWITCH_6_OPID_6_ADDR] = Control.newOPID;
		ActNode.NodeReg[SWITCH_6_STATUS_ADDR] = 0;
		ActNode.NodeReg[SWITCH_6_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			SW_Control[SW_NUM6].SWCmd = SW_STOP;
		}
		else if(Control.nControl == CONTROL_ON ) {
			SW_Control[SW_NUM6].SWCmd = SW_ON;
		}
		else if(Control.nControl == CONTROL_TIMED_ON ) {
			SW_Control[SW_NUM6].SWCmd = SW_TIMED_ON;
			SW_Control[SW_NUM6].nHoldingTime = Control.nOprTime;
		}
		Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
	}
	else if(Control.nSW_Num == SW_NUM7) {
		ActNode.NodeReg[CON_SWITCH_7_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_SWITCH_7_OPID_7_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_SWITCH_7_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[SWITCH_7_OPID_7_ADDR] = Control.newOPID;
		ActNode.NodeReg[SWITCH_7_STATUS_ADDR] = 0;
		ActNode.NodeReg[SWITCH_7_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			SW_Control[SW_NUM7].SWCmd = SW_STOP;
		}
		else if(Control.nControl == CONTROL_ON ) {
			SW_Control[SW_NUM7].SWCmd = SW_ON;
		}
		else if(Control.nControl == CONTROL_TIMED_ON ) {
			SW_Control[SW_NUM7].SWCmd = SW_TIMED_ON;
			SW_Control[SW_NUM7].nHoldingTime = Control.nOprTime;
		}
		Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
	}
	else if(Control.nSW_Num == SW_NUM8) {
		ActNode.NodeReg[CON_SWITCH_8_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_SWITCH_8_OPID_8_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_SWITCH_8_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[SWITCH_8_OPID_8_ADDR] = Control.newOPID;
		ActNode.NodeReg[SWITCH_8_STATUS_ADDR] = 0;
		ActNode.NodeReg[SWITCH_8_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			SW_Control[SW_NUM8].SWCmd = SW_STOP;
		}
		else if(Control.nControl == CONTROL_ON ) {
			SW_Control[SW_NUM8].SWCmd = SW_ON;
		}
		else if(Control.nControl == CONTROL_TIMED_ON ) {
			SW_Control[SW_NUM8].SWCmd = SW_TIMED_ON;
			SW_Control[SW_NUM8].nHoldingTime = Control.nOprTime;
		}
		Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
	}
	else if(Control.nSW_Num == SW_NUM9) {
		ActNode.NodeReg[CON_SWITCH_9_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_SWITCH_9_OPID_9_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_SWITCH_9_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[SWITCH_9_OPID_9_ADDR] = Control.newOPID;
		ActNode.NodeReg[SWITCH_9_STATUS_ADDR] = 0;
		ActNode.NodeReg[SWITCH_9_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			SW_Control[SW_NUM9].SWCmd = SW_STOP;
		}
		else if(Control.nControl == CONTROL_ON ) {
			SW_Control[SW_NUM9].SWCmd = SW_ON;
		}
		else if(Control.nControl == CONTROL_TIMED_ON ) {
			SW_Control[SW_NUM9].SWCmd = SW_TIMED_ON;
			SW_Control[SW_NUM9].nHoldingTime = Control.nOprTime;
		}
		Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
	}
	else if(Control.nSW_Num == SW_NUM10) {
		ActNode.NodeReg[CON_SWITCH_10_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_SWITCH_10_OPID_10_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_SWITCH_10_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[SWITCH_10_OPID_10_ADDR] = Control.newOPID;
		ActNode.NodeReg[SWITCH_10_STATUS_ADDR] = 0;
		ActNode.NodeReg[SWITCH_10_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			SW_Control[SW_NUM10].SWCmd = SW_STOP;
		}
		else if(Control.nControl == CONTROL_ON ) {
			SW_Control[SW_NUM10].SWCmd = SW_ON;
		}
		else if(Control.nControl == CONTROL_TIMED_ON ) {
			SW_Control[SW_NUM10].SWCmd = SW_TIMED_ON;
			SW_Control[SW_NUM10].nHoldingTime = Control.nOprTime;
		}
		Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
	}
	else if(Control.nSW_Num == SW_NUM11) {
		ActNode.NodeReg[CON_SWITCH_11_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_SWITCH_11_OPID_11_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_SWITCH_11_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[SWITCH_11_OPID_11_ADDR] = Control.newOPID;
		ActNode.NodeReg[SWITCH_11_STATUS_ADDR] = 0;
		ActNode.NodeReg[SWITCH_11_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			SW_Control[SW_NUM11].SWCmd = SW_STOP;
		}
		else if(Control.nControl == CONTROL_ON ) {
			SW_Control[SW_NUM11].SWCmd = SW_ON;
		}
		else if(Control.nControl == CONTROL_TIMED_ON ) {
			SW_Control[SW_NUM11].SWCmd = SW_TIMED_ON;
			SW_Control[SW_NUM11].nHoldingTime = Control.nOprTime;
		}
		Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
	}
	else if(Control.nSW_Num == SW_NUM12) {
		ActNode.NodeReg[CON_SWITCH_12_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_SWITCH_12_OPID_12_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_SWITCH_12_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[SWITCH_12_OPID_12_ADDR] = Control.newOPID;
		ActNode.NodeReg[SWITCH_12_STATUS_ADDR] = 0;
		ActNode.NodeReg[SWITCH_12_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			SW_Control[SW_NUM12].SWCmd = SW_STOP;
		}
		else if(Control.nControl == CONTROL_ON ) {
			SW_Control[SW_NUM12].SWCmd = SW_ON;
		}
		else if(Control.nControl == CONTROL_TIMED_ON ) {
			SW_Control[SW_NUM12].SWCmd = SW_TIMED_ON;
			SW_Control[SW_NUM12].nHoldingTime = Control.nOprTime;
		}
		Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
	}
	else if(Control.nSW_Num == SW_NUM13) {
		ActNode.NodeReg[CON_SWITCH_13_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_SWITCH_13_OPID_13_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_SWITCH_13_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[SWITCH_13_OPID_13_ADDR] = Control.newOPID;
		ActNode.NodeReg[SWITCH_13_STATUS_ADDR] = 0;
		ActNode.NodeReg[SWITCH_13_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			SW_Control[SW_NUM13].SWCmd = SW_STOP;
		}
		else if(Control.nControl == CONTROL_ON ) {
			SW_Control[SW_NUM13].SWCmd = SW_ON;
		}
		else if(Control.nControl == CONTROL_TIMED_ON ) {
			SW_Control[SW_NUM13].SWCmd = SW_TIMED_ON;
			SW_Control[SW_NUM13].nHoldingTime = Control.nOprTime;
		}
		Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
	}
	else if(Control.nSW_Num == SW_NUM14) {
		ActNode.NodeReg[CON_SWITCH_14_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_SWITCH_14_OPID_14_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_SWITCH_14_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[SWITCH_14_OPID_14_ADDR] = Control.newOPID;
		ActNode.NodeReg[SWITCH_14_STATUS_ADDR] = 0;
		ActNode.NodeReg[SWITCH_14_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			SW_Control[SW_NUM14].SWCmd = SW_STOP;
		}
		else if(Control.nControl == CONTROL_ON ) {
			SW_Control[SW_NUM14].SWCmd = SW_ON;
		}
		else if(Control.nControl == CONTROL_TIMED_ON ) {
			SW_Control[SW_NUM14].SWCmd = SW_TIMED_ON;
			SW_Control[SW_NUM14].nHoldingTime = Control.nOprTime;
		}
		Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
	}
	else if(Control.nSW_Num == SW_NUM15) {
		ActNode.NodeReg[CON_SWITCH_15_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_SWITCH_15_OPID_15_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_SWITCH_15_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[SWITCH_15_OPID_15_ADDR] = Control.newOPID;
		ActNode.NodeReg[SWITCH_15_STATUS_ADDR] = 0;
		ActNode.NodeReg[SWITCH_15_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			SW_Control[SW_NUM15].SWCmd = SW_STOP;
		}
		else if(Control.nControl == CONTROL_ON ) {
			SW_Control[SW_NUM15].SWCmd = SW_ON;
		}
		else if(Control.nControl == CONTROL_TIMED_ON ) {
			SW_Control[SW_NUM15].SWCmd = SW_TIMED_ON;
			SW_Control[SW_NUM15].nHoldingTime = Control.nOprTime;
		}
		Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
	}
	else if(Control.nSW_Num == SW_NUM16) {
		ActNode.NodeReg[CON_SWITCH_16_CMD_ADDR] = Control.nControl;
		ActNode.NodeReg[CON_SWITCH_16_OPID_16_ADDR] = Control.newOPID;
		ActNode.NodeReg[CON_SWITCH_16_TIME_ADDR] = Control.nOprTime;

		ActNode.NodeReg[SWITCH_16_OPID_16_ADDR] = Control.newOPID;
		ActNode.NodeReg[SWITCH_16_STATUS_ADDR] = 0;
		ActNode.NodeReg[SWITCH_16_TIME_ADDR] = Control.nOprTime;

		if(Control.nControl == CONTROL_STOP ) {
			SW_Control[SW_NUM16].SWCmd = SW_STOP;
		}
		else if(Control.nControl == CONTROL_ON ) {
			SW_Control[SW_NUM16].SWCmd = SW_ON;
		}
		else if(Control.nControl == CONTROL_TIMED_ON ) {
			SW_Control[SW_NUM16].SWCmd = SW_TIMED_ON;
			SW_Control[SW_NUM16].nHoldingTime = Control.nOprTime;
		}
		Actuator_SW_Control.Step = SW_RELAY_STEP_WRITE_CMD;
	}
}


//===========================================================================
void Master_Write_Multiple_Ack_Process(uint8_t* pPacket)
{
	uint8_t		nIndex = 0;

	Master_Ring_Buffer.TxBuffer[nIndex++] = (uint8_t)ActNode.nNodeID;   	// Slave Address
	Master_Ring_Buffer.TxBuffer[nIndex++] = MODBUS_WRITE_MULTIPLE_COMMAND;    // Modbus Read Holding Registers Command

	Master_Ring_Buffer.TxBuffer[nIndex++] = pPacket[2];		// Address
	Master_Ring_Buffer.TxBuffer[nIndex++] = pPacket[3];		// Address
	Master_Ring_Buffer.TxBuffer[nIndex++] = pPacket[4];    	// Quantity
	Master_Ring_Buffer.TxBuffer[nIndex++] = pPacket[5];    	// Quantity

	Master_Crc_Check((volatile uint8_t *)Master_Ring_Buffer.TxBuffer, nIndex);
	Master_Ring_Buffer.TxBuffer[nIndex++] = MstCRC1;
	Master_Ring_Buffer.TxBuffer[nIndex++] = MstCRC2;

	MST_LED_ON;
	GHC_TX_Enable;
		HAL_UART_Transmit(&huart1, Master_Ring_Buffer.TxBuffer, nIndex, 1500);
	MST_LED_OFF;
	GHC_RX_Enable;

}
//=========================================================================================================
void Master_ActNode_ID_Ack_Process(void)
{
	uint8_t		nIndex = 0;
	uint8_t 	hiData, lowData;

	Master_Ring_Buffer.TxBuffer[nIndex++] = (uint8_t)ActNode.nNodeID;   	// Slave Address
	Master_Ring_Buffer.TxBuffer[nIndex++] = MODBUS_READ_INPUT_COMMAND;    // Modbus Read Input Registers Command

	Master_Ring_Buffer.TxBuffer[nIndex++] = 0x02;    		// Byte Count
	hiData =  (uint8_t)(ActNode.nNodeID >> 8);
	lowData = (uint8_t)(ActNode.nNodeID);
	Master_Ring_Buffer.TxBuffer[nIndex++] = hiData;    		// Data
	Master_Ring_Buffer.TxBuffer[nIndex++] = lowData;    	// Data

	Master_Crc_Check((volatile uint8_t *)Master_Ring_Buffer.TxBuffer, nIndex);
	Master_Ring_Buffer.TxBuffer[nIndex++] = MstCRC1;
	Master_Ring_Buffer.TxBuffer[nIndex++] = MstCRC2;

	MST_LED_ON;
	GHC_TX_Enable;
		HAL_UART_Transmit(&huart1, Master_Ring_Buffer.TxBuffer, nIndex, 1500);
	MST_LED_OFF;
	GHC_RX_Enable;

}
//==============================================================================================================
void Master_ActNode_Baud_Ack_Process(void)
{
	uint8_t		nIndex = 0;
	uint8_t 	hiData, lowData;

	Master_Ring_Buffer.TxBuffer[nIndex++] = (uint8_t)ActNode.nNodeID;  		// Slave Address
	Master_Ring_Buffer.TxBuffer[nIndex++] = MODBUS_READ_INPUT_COMMAND;    // Modbus Read Input Registers Command

	Master_Ring_Buffer.TxBuffer[nIndex++] = 0x02;    		// Byte Count
	hiData =  (uint8_t)(Operate_Config.nNode485Baud >> 8);
	lowData = (uint8_t)(Operate_Config.nNode485Baud);
	Master_Ring_Buffer.TxBuffer[nIndex++] = hiData;    		// Data
	Master_Ring_Buffer.TxBuffer[nIndex++] = lowData;    	// Data

	Master_Crc_Check((volatile uint8_t *)Master_Ring_Buffer.TxBuffer, nIndex);
	Master_Ring_Buffer.TxBuffer[nIndex++] = MstCRC1;
	Master_Ring_Buffer.TxBuffer[nIndex++] = MstCRC2;

	MST_LED_ON;
	GHC_TX_Enable;
		HAL_UART_Transmit(&huart1, Master_Ring_Buffer.TxBuffer, nIndex, 1500);
	MST_LED_OFF;
	GHC_RX_Enable;

}

void Master_Actuator_Baud_Ack_Process(void)
{
	uint8_t		nIndex = 0;
	uint8_t 	hiData, lowData;

	Master_Ring_Buffer.TxBuffer[nIndex++] = (uint8_t)ActNode.nNodeID;  	// Slave Address
	Master_Ring_Buffer.TxBuffer[nIndex++] = MODBUS_READ_INPUT_COMMAND;    	// Modbus Read Input Registers Command

	Master_Ring_Buffer.TxBuffer[nIndex++] = 0x02;    		// Byte Count
	hiData =  (uint8_t)(Operate_Config.nAct485Baud >> 8);
	lowData = (uint8_t)(Operate_Config.nAct485Baud);
	Master_Ring_Buffer.TxBuffer[nIndex++] = hiData;    		// Data
	Master_Ring_Buffer.TxBuffer[nIndex++] = lowData;    	// Data

	Master_Crc_Check((volatile uint8_t *)Master_Ring_Buffer.TxBuffer, nIndex);
	Master_Ring_Buffer.TxBuffer[nIndex++] = MstCRC1;
	Master_Ring_Buffer.TxBuffer[nIndex++] = MstCRC2;

	MST_LED_ON;
	GHC_TX_Enable;
		HAL_UART_Transmit(&huart1, Master_Ring_Buffer.TxBuffer, nIndex, 1500);
	MST_LED_OFF;
	GHC_RX_Enable;

}

//==============================================================================================================
void Master_ActNode_Baud_Change(uint16_t nBaud)
{
	HAL_UART_Abort_IT(&huart1);

	HAL_UART_DeInit(&huart1);

	if(nBaud == 0x01) {
		huart1.Instance = USART1;
		huart1.Init.BaudRate = 9600;
		huart1.Init.WordLength = UART_WORDLENGTH_8B;
		huart1.Init.StopBits = UART_STOPBITS_1;
		huart1.Init.Parity = UART_PARITY_NONE;
		huart1.Init.Mode = UART_MODE_TX_RX;
		huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
		huart1.Init.OverSampling = UART_OVERSAMPLING_16;
		huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
		huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

		Debug_printf(" ActNode Baudrate : 9600 bps \n");
	}
	else if(nBaud == 0x02) {
		huart1.Instance = USART1;
		huart1.Init.BaudRate = 19200;
		huart1.Init.WordLength = UART_WORDLENGTH_8B;
		huart1.Init.StopBits = UART_STOPBITS_1;
		huart1.Init.Parity = UART_PARITY_NONE;
		huart1.Init.Mode = UART_MODE_TX_RX;
		huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
		huart1.Init.OverSampling = UART_OVERSAMPLING_16;
		huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
		huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

		Debug_printf(" ActNode Baudrate : 19200 bps \n");
	}
	else if(nBaud == 0x03) {
		huart1.Instance = USART1;
		huart1.Init.BaudRate = 38400;
		huart1.Init.WordLength = UART_WORDLENGTH_8B;
		huart1.Init.StopBits = UART_STOPBITS_1;
		huart1.Init.Parity = UART_PARITY_NONE;
		huart1.Init.Mode = UART_MODE_TX_RX;
		huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
		huart1.Init.OverSampling = UART_OVERSAMPLING_16;
		huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
		huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

		Debug_printf(" ActNode Baudrate : 38400 bps \n");
	}
	else if(nBaud == 0x04) {
		huart1.Instance = USART1;
		huart1.Init.BaudRate = 57600;
		huart1.Init.WordLength = UART_WORDLENGTH_8B;
		huart1.Init.StopBits = UART_STOPBITS_1;
		huart1.Init.Parity = UART_PARITY_NONE;
		huart1.Init.Mode = UART_MODE_TX_RX;
		huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
		huart1.Init.OverSampling = UART_OVERSAMPLING_16;
		huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
		huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

		Debug_printf(" ActNode Baudrate : 57600 bps \n");
	}
	else if(nBaud == 0x05) {
		huart1.Instance = USART1;
		huart1.Init.BaudRate = 115200;
		huart1.Init.WordLength = UART_WORDLENGTH_8B;
		huart1.Init.StopBits = UART_STOPBITS_1;
		huart1.Init.Parity = UART_PARITY_NONE;
		huart1.Init.Mode = UART_MODE_TX_RX;
		huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
		huart1.Init.OverSampling = UART_OVERSAMPLING_16;
		huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
		huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

		Debug_printf(" ActNode Baudrate : 115200 bps \n");
	}

	if (HAL_RS485Ex_Init(&huart1, UART_DE_POLARITY_HIGH, 0, 0) != HAL_OK)
	{
		Error_Handler();
	}

	HAL_UART_Receive_IT(&huart1, &Master_Ring_Buffer.RxData, 1);
}
void Master_ActNode_Baud_Change_Process(uint8_t* pPacket, uint8_t nSize)
{
	uint8_t		nIndex = 0;

	Install_Config.nNode485Baud = (uint16_t)((pPacket[4] << 8) + pPacket[5]);

	if( 0x00 < Install_Config.nNode485Baud && Install_Config.nNode485Baud < 0x06) {

		Master_Ring_Buffer.TxBuffer[nIndex++] = (uint8_t)ActNode.nNodeID;  	// Slave Address
		Master_Ring_Buffer.TxBuffer[nIndex++] = MODBUS_WRITE_SINGLE_COMMAND;    	// Write Single Registers Command

		Master_Ring_Buffer.TxBuffer[nIndex++] = pPacket[2]; 	// Register Addr
		Master_Ring_Buffer.TxBuffer[nIndex++] = pPacket[3]; 	// Register Addr
		Master_Ring_Buffer.TxBuffer[nIndex++] = pPacket[4]; 	// Data
		Master_Ring_Buffer.TxBuffer[nIndex++] = pPacket[5];    	// Data

		Master_Crc_Check((volatile uint8_t *)Master_Ring_Buffer.TxBuffer, nIndex);
		Master_Ring_Buffer.TxBuffer[nIndex++] = MstCRC1;
		Master_Ring_Buffer.TxBuffer[nIndex++] = MstCRC2;

		MST_LED_ON;
		GHC_TX_Enable;
		HAL_UART_Transmit(&huart1, Master_Ring_Buffer.TxBuffer, nIndex, 1500);
		MST_LED_OFF;
		GHC_RX_Enable;

		Master_ActNode_Baud_Change(Install_Config.nNode485Baud);

		Install_Config.nAct485Baud = Operate_Config.nAct485Baud;

		E2P_Default_Config_Load(&Default_Config);
		E2P_Erase();
		E2P_Default_Config_Save(&Default_Config);
		E2P_Install_Config_Save(&Install_Config);
		E2P_Write_Byte(CONFIG_CHECK_ADDR, INSTALL_WRITE_OK);

		NVIC_SystemReset();

	}
	else {
		Master_Ring_Buffer.TxBuffer[nIndex++] = (uint8_t)ActNode.nNodeID;    // Slave Address
		Master_Ring_Buffer.TxBuffer[nIndex++] = MODBUS_WRITE_SINGLE_COMMAND + 0x80;   // Write Single Registers Command

		Master_Ring_Buffer.TxBuffer[nIndex++] = 0x02;    		// Exception Code

		Master_Crc_Check((volatile uint8_t *)Master_Ring_Buffer.TxBuffer, nIndex);
		Master_Ring_Buffer.TxBuffer[nIndex++] = MstCRC1;
		Master_Ring_Buffer.TxBuffer[nIndex++] = MstCRC2;

		MST_LED_ON;
		GHC_TX_Enable;
			HAL_UART_Transmit(&huart1, Master_Ring_Buffer.TxBuffer, nIndex, 1500);
		MST_LED_OFF;
		GHC_RX_Enable;
	}
}
//==============================================================================================================
void Master_Actuator_Baud_Change(uint16_t nBaud)
{
	HAL_UART_Abort_IT(&huart3);

	HAL_UART_DeInit(&huart3);

	if(nBaud == 0x01) {
		huart3.Instance = USART3;
		huart3.Init.BaudRate = 9600;
		huart3.Init.WordLength = UART_WORDLENGTH_8B;
		huart3.Init.StopBits = UART_STOPBITS_1;
		huart3.Init.Parity = UART_PARITY_NONE;
		huart3.Init.Mode = UART_MODE_TX_RX;
		huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
		huart3.Init.OverSampling = UART_OVERSAMPLING_16;
		huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
		huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

		Debug_printf(" Actuator Baudrate : 9600 bps \n");
	}
	else if(nBaud == 0x02) {
		huart3.Instance = USART3;
		huart3.Init.BaudRate = 19200;
		huart3.Init.WordLength = UART_WORDLENGTH_8B;
		huart3.Init.StopBits = UART_STOPBITS_1;
		huart3.Init.Parity = UART_PARITY_NONE;
		huart3.Init.Mode = UART_MODE_TX_RX;
		huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
		huart3.Init.OverSampling = UART_OVERSAMPLING_16;
		huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
		huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

		Debug_printf(" Actuator Baudrate : 19200 bps \n");
	}
	else if(nBaud == 0x03) {
		huart3.Instance = USART3;
		huart3.Init.BaudRate = 38400;
		huart3.Init.WordLength = UART_WORDLENGTH_8B;
		huart3.Init.StopBits = UART_STOPBITS_1;
		huart3.Init.Parity = UART_PARITY_NONE;
		huart3.Init.Mode = UART_MODE_TX_RX;
		huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
		huart3.Init.OverSampling = UART_OVERSAMPLING_16;
		huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
		huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

		Debug_printf(" Actuator Baudrate : 38400 bps \n");
	}
	else if(nBaud == 0x04) {
		huart3.Instance = USART3;
		huart3.Init.BaudRate = 57600;
		huart3.Init.WordLength = UART_WORDLENGTH_8B;
		huart3.Init.StopBits = UART_STOPBITS_1;
		huart3.Init.Parity = UART_PARITY_NONE;
		huart3.Init.Mode = UART_MODE_TX_RX;
		huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
		huart3.Init.OverSampling = UART_OVERSAMPLING_16;
		huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
		huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

		Debug_printf(" Actuator Baudrate : 57600 bps \n");
	}
	else if(nBaud == 0x05) {
		huart3.Instance = USART3;
		huart3.Init.BaudRate = 115200;
		huart3.Init.WordLength = UART_WORDLENGTH_8B;
		huart3.Init.StopBits = UART_STOPBITS_1;
		huart3.Init.Parity = UART_PARITY_NONE;
		huart3.Init.Mode = UART_MODE_TX_RX;
		huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
		huart3.Init.OverSampling = UART_OVERSAMPLING_16;
		huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
		huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

		Debug_printf(" Actuator Baudrate : 115200 bps \n");
	}

	if (HAL_UART_Init(&huart3) != HAL_OK) {
		Error_Handler();
	}

	HAL_UART_Receive_IT(&huart3, &Actuator_Ring_Buffer.RxData, 1);
}

void Master_Actuator_Baud_Change_Process(uint8_t* pPacket, uint8_t nSize)
{
	uint8_t		nIndex = 0;

	Install_Config.nAct485Baud = (uint16_t)((pPacket[4] << 8) + pPacket[5]);

	if( 0x00 < Install_Config.nAct485Baud && Install_Config.nAct485Baud < 0x06) {

		Master_Ring_Buffer.TxBuffer[nIndex++] = (uint8_t)ActNode.nNodeID;  	// Slave Address
		Master_Ring_Buffer.TxBuffer[nIndex++] = MODBUS_WRITE_SINGLE_COMMAND;    	// Write Single Registers Command

		Master_Ring_Buffer.TxBuffer[nIndex++] = pPacket[2]; 	// Register Addr
		Master_Ring_Buffer.TxBuffer[nIndex++] = pPacket[3]; 	// Register Addr
		Master_Ring_Buffer.TxBuffer[nIndex++] = pPacket[4]; 	// Data
		Master_Ring_Buffer.TxBuffer[nIndex++] = pPacket[5];    	// Data

		Master_Crc_Check((volatile uint8_t *)Master_Ring_Buffer.TxBuffer, nIndex);
		Master_Ring_Buffer.TxBuffer[nIndex++] = MstCRC1;
		Master_Ring_Buffer.TxBuffer[nIndex++] = MstCRC2;

		MST_LED_ON;
		GHC_TX_Enable;
		HAL_UART_Transmit(&huart1, Master_Ring_Buffer.TxBuffer, nIndex, 1500);
		MST_LED_OFF;
		GHC_RX_Enable;

		Master_ActNode_Baud_Change(Install_Config.nNode485Baud);

		Install_Config.nNode485Baud = Operate_Config.nNode485Baud;

		E2P_Default_Config_Load(&Default_Config);
		E2P_Erase();
		E2P_Default_Config_Save(&Default_Config);
		E2P_Install_Config_Save(&Install_Config);
		E2P_Write_Byte(CONFIG_CHECK_ADDR, INSTALL_WRITE_OK);

		NVIC_SystemReset();

	}
	else {
		Master_Ring_Buffer.TxBuffer[nIndex++] = (uint8_t)ActNode.nNodeID;    			// Slave Address
		Master_Ring_Buffer.TxBuffer[nIndex++] = MODBUS_WRITE_SINGLE_COMMAND + 0x80;   	// Write Single Registers Command

		Master_Ring_Buffer.TxBuffer[nIndex++] = 0x02;    		// Byte Count

		Master_Crc_Check((volatile uint8_t *)Master_Ring_Buffer.TxBuffer, nIndex);
		Master_Ring_Buffer.TxBuffer[nIndex++] = MstCRC1;
		Master_Ring_Buffer.TxBuffer[nIndex++] = MstCRC2;

		MST_LED_ON;
		GHC_TX_Enable;
			HAL_UART_Transmit(&huart1, Master_Ring_Buffer.TxBuffer, nIndex, 1500);
		MST_LED_OFF;
		GHC_RX_Enable;
	}
}

//==============================================================================================================
void Master_Factory_Reset_Process(uint8_t* pPacket, uint8_t nSize)
{
	uint8_t		nIndex = 0;
	uint16_t 	uData;

	Master_Ring_Buffer.TxBuffer[nIndex++] = (uint8_t)ActNode.nNodeID;    // Slave Address
	Master_Ring_Buffer.TxBuffer[nIndex++] = MODBUS_WRITE_SINGLE_COMMAND;    	// Modbus Write Single Registers Command

	Master_Ring_Buffer.TxBuffer[nIndex++] = pPacket[2]; 	// Register Addr
	Master_Ring_Buffer.TxBuffer[nIndex++] = pPacket[3]; 	// Register Addr
	Master_Ring_Buffer.TxBuffer[nIndex++] = pPacket[4]; 	// Data
	Master_Ring_Buffer.TxBuffer[nIndex++] = pPacket[5];    	// Data

	uData = (uint16_t)((pPacket[4] << 8) + pPacket[5]);
	Master_Crc_Check((volatile uint8_t *)Master_Ring_Buffer.TxBuffer, nIndex);
	Master_Ring_Buffer.TxBuffer[nIndex++] = MstCRC1;
	Master_Ring_Buffer.TxBuffer[nIndex++] = MstCRC2;

	MST_LED_ON;
	GHC_TX_Enable;
		HAL_UART_Transmit(&huart1, Master_Ring_Buffer.TxBuffer, nIndex, 1500);
	MST_LED_OFF;
	GHC_RX_Enable;

	if(uData == 0x0004) {
		E2P_Default_Config_Load(&Default_Config);
		E2P_Erase();
		E2P_Default_Config_Save(&Default_Config);
		E2P_Write_Byte(CONFIG_CHECK_ADDR, DEFAULT_WRITE_OK);

		NVIC_SystemReset();
	}
	else if(uData == 0x0003) {

		NVIC_SystemReset();
	}
}

void Master_Exception_0x01_Ack_Process(uint8_t Command)
{
	uint8_t		nIndex = 0;

	Master_Ring_Buffer.TxBuffer[nIndex++] = (uint8_t)ActNode.nNodeID;    			   // Slave Address
	Master_Ring_Buffer.TxBuffer[nIndex++] = Command + 0x80;    // Modbus Read Holding Registers Command

	Master_Ring_Buffer.TxBuffer[nIndex++] = 0x01;    								// Error

	Master_Crc_Check((volatile uint8_t *)Master_Ring_Buffer.TxBuffer, nIndex);
	Master_Ring_Buffer.TxBuffer[nIndex++] = MstCRC1;
	Master_Ring_Buffer.TxBuffer[nIndex++] = MstCRC2;

	MST_LED_ON;
	GHC_TX_Enable;
		HAL_UART_Transmit(&huart1, Master_Ring_Buffer.TxBuffer, nIndex, 1500);
	MST_LED_OFF;
	GHC_RX_Enable;

}
void Master_Exception_0x02_Ack_Process(uint8_t Command)
{
	uint8_t		nIndex = 0;

	Master_Ring_Buffer.TxBuffer[nIndex++] = (uint8_t)ActNode.nNodeID;    			   // Slave Address
	Master_Ring_Buffer.TxBuffer[nIndex++] = Command + 0x80;    // Modbus Read Holding Registers Command

	Master_Ring_Buffer.TxBuffer[nIndex++] = 0x02;    								// Error

	Master_Crc_Check((volatile uint8_t *)Master_Ring_Buffer.TxBuffer, nIndex);
	Master_Ring_Buffer.TxBuffer[nIndex++] = MstCRC1;
	Master_Ring_Buffer.TxBuffer[nIndex++] = MstCRC2;

	MST_LED_ON;
	GHC_TX_Enable;
		HAL_UART_Transmit(&huart1, Master_Ring_Buffer.TxBuffer, nIndex, 1500);
	MST_LED_OFF;
	GHC_RX_Enable;
}

void Master_Exception_0x03_Ack_Process(uint8_t Command)
{
	uint8_t		nIndex = 0;

	Master_Ring_Buffer.TxBuffer[nIndex++] = (uint8_t)ActNode.nNodeID;    			   // Slave Address
	Master_Ring_Buffer.TxBuffer[nIndex++] = Command + 0x80;

	Master_Ring_Buffer.TxBuffer[nIndex++] = 0x03;    		// Error

	Master_Crc_Check((volatile uint8_t *)Master_Ring_Buffer.TxBuffer, nIndex);
	Master_Ring_Buffer.TxBuffer[nIndex++] = MstCRC1;
	Master_Ring_Buffer.TxBuffer[nIndex++] = MstCRC2;

	MST_LED_ON;
	GHC_TX_Enable;
		HAL_UART_Transmit(&huart1, Master_Ring_Buffer.TxBuffer, nIndex, 1500);
	MST_LED_OFF;
	GHC_RX_Enable;
}

//=========================================================================================================
void Master_Node_Ack_Process(uint16_t RequestAddr, uint16_t RequestDataNo)
{
	uint16_t	nIndex = 0;
	uint8_t 	hiData, lowData;
	uint16_t	cnt;

	Master_Ring_Buffer.TxBuffer[nIndex++] =  (uint8_t)ActNode.nNodeID;	// Slave Address
	Master_Ring_Buffer.TxBuffer[nIndex++] = MODBUS_READ_HOLDING_COMMAND;    // Modbus Read Holding Registers Command

	Master_Ring_Buffer.TxBuffer[nIndex++] = (RequestDataNo * 2);    		// Byte Count

	for(cnt = RequestAddr; cnt < (RequestAddr + RequestDataNo); cnt++)
	{
		hiData = (uint8_t)(ActNode.NodeReg[cnt] >> 8);
		lowData = (uint8_t)(ActNode.NodeReg[cnt]);

		Master_Ring_Buffer.TxBuffer[nIndex++] = hiData;
		Master_Ring_Buffer.TxBuffer[nIndex++] = lowData;
	}

	Master_Crc_Check((volatile uint8_t *)Master_Ring_Buffer.TxBuffer, nIndex);
	Master_Ring_Buffer.TxBuffer[nIndex++] = MstCRC1;
	Master_Ring_Buffer.TxBuffer[nIndex++] = MstCRC2;

#if MASTER_DEBUG
	for(cnt = 0; cnt < nIndex; cnt++) {
		Debug_printf("\n%d : %02x",cnt, Master_Ring_Buffer.TxBuffer[cnt]);
	}
#endif

#if NETWORK_OUTPUT
	if(RequestAddr == 0x00C9) {
		ActuatorNode_NetworkSend_Process(0x01);
	}
#endif

	MST_LED_ON;
	GHC_TX_Enable;
		HAL_UART_Transmit(&huart1, Master_Ring_Buffer.TxBuffer, nIndex, 1500);
	MST_LED_OFF;
	GHC_RX_Enable;
}

//===========================================================================================
void Master_1ms_Process(void)
{
    if(ActNode.nRxEndTime != 0) {
    	ActNode.nRxEndTime--;
    }

    if(ActNode.nHoldWaitTime != 0xFFFFFFFF) {
    	ActNode.nHoldWaitTime++;
    }
}

//=========================================================================
void Master_OPID_Reset_Process(void)
{
	for(int i = 0; i < 16; i++)
	{
		// SW real variable
		SW_Control[i].preOPID = 0;
		SW_Control[i].newOPID = 0;

		// SW Master Comm. variable
		ActNode.NodeReg[CON_SWITCH_1_OPID_1_ADDR + (i * 4)] = 0;
		ActNode.NodeReg[SWITCH_1_OPID_1_ADDR + (i * 4)] = 0;

		// OCS real variable
		OCS_Control[i].preOPID = 0;
		OCS_Control[i].newOPID = 0;

		// OSC Master Comm. variable
		ActNode.NodeReg[CON_OCSWITCH_1_OPID_17_ADDR + (i * 4)] = 0;
		ActNode.NodeReg[OCSWITCH_1_OPID_17_ADDR + (i * 4)] = 0;
	}
}

//=========================================================================
uint32_t VDC_ADC_Avgerge(ADCBAT_INFO AdcAVG)
{
	uint16_t average = 0;
	uint32_t data_sum = 0;
	uint8_t	cnt = 0;

	for(cnt = 0; cnt < ADC_BUFFER_SIZE; cnt++) {
		data_sum += (AdcAVG.Adc_Buffer[cnt] - ADC_MINUSOFFSET);
	}
	average = (uint16_t)(data_sum / 10);

	return average;
}

//=========================================================  Network Output
int SW_Get(){
	return Actuator_SW_Control.nReadRelay;
}

int R1_Get(){
	return Actuator_OCS_Control.nReadRelay;
}

int R2_Get(){
	return Actuator_OCS_EXT_Control.nReadRelay;
}

void ActuatorNode_NetworkSend_Process(uint8_t flag)
{
#if NETWORK_OUTPUT
	char strBuffer[200];

	AdcVDC.Adc_Value = (uint32_t)(((VDC_ADC_Avgerge(AdcVDC) * ADC_REF_33_VOLT)/4095) );
	AdcVDC.fDCVoltage =  (float)(AdcVDC.Adc_Value / 1000.0f / 0.087f );

	sprintf(strBuffer,"NODE:%02d,%4s,%04X,%04X,%04X,%0d,%0.1f,%d\n"
			,ActNode.nNodeID,"00A4",
			SW_Get( ),R1_Get( ),R2_Get(),ActNode.NodeReg[CON_ACTNODE_OPID_0_ADDR],
			AdcVDC.fDCVoltage,flag);

	HAL_UART_Transmit(&huart2, (uint8_t *)strBuffer, strlen(strBuffer), 1500);
#endif

}

