/*
 * PTM_Process.h
 *
 *  Created on: 2021. 4. 15.
 *      Author: CoreSolution
 */

#ifndef ACTUATOR_PROCESS_H_
#define ACTUATOR_PROCESS_H_

#include "define.h"
#include "struct.h"

uint16_t Crc_Check(volatile uint8_t *nData, uint16_t wLength);
uint16_t Master_Crc_Check(volatile uint8_t *nData, uint16_t wLength);

void Actuator_RS485_Push_Data(uint8_t nData);
uint8_t Actuator_RS485_Pop_Data(void);
void Actuator_Buffer_Process(void);
void Actuator_Packet_Process(uint8_t* pPacket, uint8_t nSize);

uint16_t Actuator_Get_NodeID(void);
void Actuator_Install_Initialize(void);

void Actuator_OCS_Relay_Get_Tx_Data(void);
void Actuator_OCS_Relay_Set_Tx_Data(uint32_t wRelayValue);
void Actuator_OCS_EXT_Relay_Get_Tx_Data(void);
void Actuator_OCS_EXT_Relay_Set_Tx_Data(uint32_t wRelayValue);
void Actuator_SW_Relay_Get_Tx_Data(void);
void Actuator_SW_Relay_Set_Tx_Data(uint16_t wRelayValue);

void Actuator_OCS_Relay_Control_Proecess(void);
void Actuator_OCS_Next_Step(OCS_RELAY_STEP step);

void Actuator_OCS_EXT_Relay_Control_Proecess(void);
void Actuator_OCS_EXT_Next_Step(OCS_EXT_RELAY_STEP step);

void Actuator_SW_Relay_Control_Proecess(void);
void Actuator_SW_Next_Step(SW_RELAY_STEP step);

void Actuator_1ms_Process(void);

void Debug_PutStr(char* pString);
void Debug_printf(const char* fmt, ...);

#endif /* ENVSENSOR_SENSOR_PROCESS_H_ */
