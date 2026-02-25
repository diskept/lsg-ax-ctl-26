/*
 * Master_Process.h
 *
 *  Created on: 2021. 7. 19.
 *      Author: CoreSolution
 */

#ifndef MASTER_PROCESS_H_
#define MASTER_PROCESS_H_

#include "define.h"
#include "struct.h"

void Master_RS485_Push_Data(uint8_t nData);
uint8_t Master_RS485_Pop_Data(void);
void Master_Buffer_Process(void);
void Master_Packet_Process(uint8_t* pPacket, uint16_t nSize);

void Master_NodeData_Initialize(void);
void Master_1ms_Process(void);
void Master_OPID_Reset_Process(void);

void Master_Node_Ack_Process(uint16_t RequestAddr, uint16_t RequestDataNo);
void Master_ActNode_ID_Ack_Process(void);
void Master_ActNode_Baud_Ack_Process(void);
void Master_Actuator_Baud_Ack_Process(void);
void Master_Install_Code_Ack_Process(void);

void Master_ActNode_Baud_Change(uint16_t nBaud);
void Master_ActNode_Baud_Change_Process(uint8_t* pPacket, uint8_t nSize);
void Master_Actuator_Baud_Change(uint16_t nBaud);
void Master_Actuator_Baud_Change_Process(uint8_t* pPacket, uint8_t nSize);
void Master_Install_Code_Change_Process(uint8_t* pPacket, uint8_t nSize);

void Master_Write_Multiple_Ack_Process(uint8_t* pPacket);

void Master_Exception_0x01_Ack_Process(uint8_t Command);
void Master_Exception_0x02_Ack_Process(uint8_t Command);
void Master_Exception_0x03_Ack_Process(uint8_t Command);
void Master_Factory_Reset_Process(uint8_t* pPacket, uint8_t nSize);

void Master_Actuator_OCS_Updata_Process(OCSWITCH_CONTROL Control);
void Master_Actuator_OCS_EXT_Updata_Process(OCSWITCH_CONTROL Control);
void Master_Actuator_OnTime_Process(void);

void Master_Actuator_SW_Updata_Process(SWITCH_CONTROL Control);


void ActuatorNode_NetworkSend_Process(uint8_t flag);



#endif
