///-------------------------------------------------------------
///
///
///-------------------------------------------------------------

#ifndef __STRUCT_H
#define __STRUCT_H

#include "define.h"

//=======================================================================  data change
typedef union _ulong
{
    uint32_t	Data;
    uint8_t		chData[4];
}ULONG;

typedef union _long
{
    int32_t		Data;
    uint8_t		chData[4];
}LONG;

typedef union _float
{
    float		fData;
    uint8_t		chData[4];
}FLOAT;


typedef union _fdata 
{
	uint8_t		array[4];
	float		fValue;
}HEXTOFLOAT;

//=======================================================================  time flag
typedef struct _Time_Flag
{
	uint8_t   	bTF_1ms;
	uint8_t   	bTF_1S;
	uint16_t	nTCount_1ms;
	uint16_t	nTCount_1S;
}TIME_FLAG;

typedef struct _sec_timer_t
{
	uint32_t 	nSecTimerTick;
	uint8_t  	bEnable;
}SEC_TIMER_T;

typedef struct _msec_timer_t
{
	uint32_t 	nMsecTimerTick;
	uint8_t 	bEnable;
}MSEC_TIMER_T;



typedef struct _gpioin_control
{
    uint8_t		bTouchL;
    uint8_t		bTouchR;
    uint8_t		bPowerFlag;

    uint32_t	nTL_CheckTime;
    uint32_t	nTR_CheckTime;
    uint32_t	nTouch_TickTime;
}TOUCH_CONTROL;

//====================================================================
typedef enum
{
    ACTUATOR_SW_RELAYCH_READ 			= 0,
	ACTUATOR_OCS_RELAYCH_READ 			= 1,
    ACTUATOR_SW_RELAY16CH_WRITE			= 2,
	ACTUATOR_OCS_RELAY16CH_WRITE		= 3,
	ACTUATOR_OCS_EXT_RELAYCH_READ		= 4,
	ACTUATOR_OCS_EXT_RELAY16CH_WRITE	= 5,
}ACTUATOR_KIND;

typedef struct _uart_ring_buffer
{
    uint8_t 	RxBuffer[UART_RX_RING_BUFFER_SIZE];
    uint16_t 	nSaveIndex;
    uint16_t 	nProcessIndex; 
	uint8_t		nSaveCnt;
	uint8_t		RxData;
	
	uint8_t 	TxBuffer[UART_TX_RING_BUFFER_SIZE];
	uint16_t	nSendIndex;
	uint8_t 	nSendCnt;
}UART_RING_BUFFER;

typedef struct _act_ring_buffer
{
    uint8_t 	RxBuffer[UART_RX_RING_BUFFER_SIZE];		// 16 -> UART_RX_RING_BUFFER_SIZE 25.12.31. Fixed by diskept
    uint16_t 	nSaveIndex;
    uint16_t 	nProcessIndex;
	uint8_t		nSaveCnt;
	uint8_t		RxData;
	
	uint32_t    nRxOverflowCnt;   						// (추가) 오버플로우 카운터 25.12.31. Added by diskept

	uint8_t 	TxBuffer[64];							// 16 -> 64 25.12.31. Fixed by diskept
	uint16_t	nSendIndex;
	uint8_t 	nSendCnt;

    ACTUATOR_KIND	Acutator;
}ACT_RING_BUFFER;


typedef enum
{
    NOTINSTALLED = 0,
	INSTALLED,
}SET_UP;



typedef struct _actuator_installed
{
	uint8_t	SW_Relay16ch;
	uint8_t	OCS_Relay16ch;
	uint8_t	OCS_Relay16ch_EXT;

    uint8_t	OCSwitch_Num1;
    uint8_t	OCSwitch_Num2;
    uint8_t	OCSwitch_Num3;
    uint8_t	OCSwitch_Num4;
    uint8_t	OCSwitch_Num5;
    uint8_t	OCSwitch_Num6;
    uint8_t	OCSwitch_Num7;
    uint8_t	OCSwitch_Num8;
    uint8_t	OCSwitch_Num9;
    uint8_t	OCSwitch_Num10;
    uint8_t	OCSwitch_Num11;
    uint8_t	OCSwitch_Num12;
    uint8_t	OCSwitch_Num13;
    uint8_t	OCSwitch_Num14;
    uint8_t	OCSwitch_Num15;
    uint8_t	OCSwitch_Num16;

    uint8_t	Switch_Num1;
    uint8_t	Switch_Num2;
    uint8_t	Switch_Num3;
    uint8_t	Switch_Num4;
    uint8_t	Switch_Num5;
    uint8_t	Switch_Num6;
    uint8_t	Switch_Num7;
    uint8_t	Switch_Num8;
    uint8_t	Switch_Num9;
    uint8_t	Switch_Num10;
    uint8_t	Switch_Num11;
    uint8_t	Switch_Num12;
    uint8_t	Switch_Num13;
    uint8_t	Switch_Num14;
    uint8_t	Switch_Num15;
    uint8_t	Switch_Num16;
}ACTUATOR_INSTALLED;

typedef enum
{
	SW_RELAY16CH_ID			= 0x01,
	OCS_RELAY16CH_ID		= 0x02,
	OCS_RELAY16CH_EXT_ID	= 0x03,
}ACTUATOR_ID;

typedef enum
{
	NOTATTACHED 			= 0,
    RESERVED 				= 0x00,
	SWITCH					= 102,
	OCSWITCH				= 112,
}ACTUATOR_INFO;

typedef enum
{
    COMMON_READY = 0,
	COMMON_ERROR,
	COMMON_BUSY,
	COMMON_VOLTAGE_ERROR,
	COMMON_CURRENT_ERROR,
	COMMON_TEMPERATURE_ERROR,
	COMMON_FUSE_ERROR,
}KS_CODE_COMMON_STATUS;

typedef enum
{
    NEED_REPLACE = 101,
	NEED_CALIBRATION = 102,
	NEED_CHECK = 103,
}KS_CODE_SENSOR_STATUS;

typedef enum
{
	COMMON_STOP				= 0,
	COMMON_ON				= 201,
	COMMON_OPENING			= 301,
	COMMON_CLOSING			= 302,
	COMMON_USER_CONTROL		= 299,
	COMMON_MANUAL_CONTROL	= 399,
}KS_CODE_ACTUATOR_STATUS;

typedef enum
{
    CONTROL_LOCAL			= 1,			// 지역제어
	CONTROL_REMOTE			= 2,			// 원격제어
	CONTROL_MANUAL			= 3,			// 수동제어
}KS_CODE_CONTROL_NODE;

typedef enum
{
    CONTROL_STOP			= 0,
	CONTROL_OFF 			= 0,			//스위치 정지
	CONTROL_ON 				= 201,			//스위치 동작
	CONTROL_TIMED_ON 		= 202,			//스위치 타이머동작
	CONTROL_DIRECTIONAL_ON 	= 203,			//
	CONTROL_OPEN 			= 301,			// 강제열릴
	CONTROL_CLOSE 			= 302,			// 강제닫힘
	CONTROL_TIMED_OPEN 		= 303,   		// 시간제열림
	CONTROL_TIMED_CLOSE 	= 304,			// 시간제닫힘
	CONTROL_SET_POSITION 	= 305,
	CONTROL_SET_CONFIG 		= 306,
}KS_CODE_CONTROL_COMMAND;
//=============================================================================================
typedef enum
{
	RELAY_READY = 0,
	RELAY_BUSY,
}RELAY_STATE;

typedef enum
{
	ACTUATOR_UNKNONE,
	ACTUATOR_SEND_OK,
	ACTUATOR_SEND_ERROR,
	ACTUATOR_RECV_OK,
	ACTUATOR_RECV_ERROR,
}ACTUATOR_STATE;

typedef enum
{
    OCS_RELAY_STEP_START = 0,           // 0

	OCS_RELAY_STEP_WRITE_CMD,			// 1
	OCS_RELAY_STEP_STATUS_READ,			// 2
	OCS_RELAY_STEP_STATUS_READ_WAIT,	// 3
	OCS_RELAY_STEP_WRITE_DATA_MASK,		// 4
	OCS_RELAY_STEP_DATA_WRITE,			// 5
	OCS_RELAY_STEP_DATA_WRITE_WAIT,		// 6
	OCS_RELAY_STEP_IDLE,				// 7
} OCS_RELAY_STEP;

typedef struct _actuator_ocs_control
{
	RELAY_STATE		RelayState;
    ACTUATOR_STATE	State;
    OCS_RELAY_STEP	Step;

    uint8_t		nRetryCount;
	uint32_t	nReadRelay;
	uint32_t	nWriteRelay;

    uint32_t    nWaitTime;
}ACTUATOR_OCS_CONTROL;

typedef enum
{
    OCS_EXT_RELAY_STEP_START = 0,           // 0

	OCS_EXT_RELAY_STEP_WRITE_CMD,			// 1
	OCS_EXT_RELAY_STEP_STATUS_READ,			// 2
	OCS_EXT_RELAY_STEP_STATUS_READ_WAIT,	// 3
	OCS_EXT_RELAY_STEP_WRITE_DATA_MASK,		// 4
	OCS_EXT_RELAY_STEP_DATA_WRITE,			// 5
	OCS_EXT_RELAY_STEP_DATA_WRITE_WAIT,		// 6
	OCS_EXT_RELAY_STEP_IDLE,				// 7
} OCS_EXT_RELAY_STEP;

typedef struct _actuator_ocs_ext_control
{
	RELAY_STATE			RelayState;
    ACTUATOR_STATE		State;
    OCS_EXT_RELAY_STEP	Step;

    uint8_t		nRetryCount;
	uint16_t	nReadRelay;
	uint16_t	nWriteRelay;

    uint32_t    nWaitTime;
}ACTUATOR_OCS_EXT_CONTROL;

typedef enum
{
    SW_RELAY_STEP_START = 0,           	// 0

	SW_RELAY_STEP_WRITE_CMD,			// 1
	SW_RELAY_STEP_STATUS_READ,			// 2
	SW_RELAY_STEP_STATUS_READ_WAIT,		// 3
	SW_RELAY_STEP_WRITE_DATA_MASK,		// 4
	SW_RELAY_STEP_DATA_WRITE,			// 5
	SW_RELAY_STEP_DATA_WRITE_WAIT,		// 6
	SW_RELAY_STEP_IDLE,					// 7
} SW_RELAY_STEP;

typedef struct _actuator_sw_control
{
	RELAY_STATE		RelayState;
    ACTUATOR_STATE	State;
    SW_RELAY_STEP	Step;

    uint8_t		nRetryCount;
	uint16_t	nReadRelay;
	uint16_t	nWriteRelay;

    uint32_t    nWaitTime;
}ACTUATOR_SW_CONTROL;

typedef struct _oprnode_control
{
	KS_CODE_COMMON_STATUS   CommonStatus;
	KS_CODE_SENSOR_STATUS   SensorStatus;

	uint16_t	nNodeID;
	uint16_t 	NodeReg[DEFAULT_REGISTER_MAX];

	uint16_t 	nRequestAddr;
	uint16_t 	nRequestDataNo;

	uint16_t	nMultiConCount;
	uint8_t		bConMode;
	uint8_t		nConCount;
	uint16_t	nRejCount;

    uint32_t	nHoldWaitTime;
    uint16_t	nRxEndTime;
}ACTNODE_CONTROL;

typedef enum
{
	SINGLE_MODE = 0,
	MULTI_MODE
} ACT_MODE;

typedef enum
{
	OCS_NUM1 = 0,
	OCS_NUM2,
	OCS_NUM3,
	OCS_NUM4,
	OCS_NUM5,
	OCS_NUM6,
	OCS_NUM7,
	OCS_NUM8,
	OCS_NUM9,
	OCS_NUM10,
	OCS_NUM11,
	OCS_NUM12,
	OCS_NUM13,
	OCS_NUM14,
	OCS_NUM15,
	OCS_NUM16,
} OCS_NUM;

typedef enum
{
	OCS_STOP = 0,
	OCS_OPEN,
	OCS_CLOSE,
	OCS_TIMED_OPEN,
	OCS_TIMED_CLOSE,
	OCS_RUNNING,
}OCS_COMMAND;

typedef enum
{
	OCS_RUN_STOP = 0,
	OCS_OPEN_START,
	OCS_CLOSE_START,
	OCS_TIME_STOP,
	OCS_TIME_OPEN_START,
	OCS_TIME_CLOSE_START,
}OCS_STATUS;

typedef struct _ocswitch_control
{
	KS_CODE_ACTUATOR_STATUS 	Status;
	KS_CODE_CONTROL_COMMAND 	Command;

	uint8_t		nOCS_Num;
	uint16_t 	nControl;
	uint16_t	newOPID;
	uint16_t	preOPID;
	uint16_t 	nStatus;
    uint32_t	nOprTime;

    uint8_t		mTMR_1Sec;
    uint32_t	nHoldingTime;
    uint32_t	nRemainingTime;
    uint32_t	nGetTime;

    OCS_STATUS	OCStatus;
	OCS_COMMAND	OCSCmd;

}OCSWITCH_CONTROL;
//=========================================================
typedef enum
{
	SW_NUM1 = 0,
	SW_NUM2,
	SW_NUM3,
	SW_NUM4,
	SW_NUM5,
	SW_NUM6,
	SW_NUM7,
	SW_NUM8,
	SW_NUM9,
	SW_NUM10,
	SW_NUM11,
	SW_NUM12,
	SW_NUM13,
	SW_NUM14,
	SW_NUM15,
	SW_NUM16,
} SW_NUM;

typedef enum
{
	SW_STOP = 0,
	SW_ON,
	SW_TIMED_ON,
	SW_RUNNING,
}SW_COMMAND;

typedef enum
{
	SW_OFF = 0,
	SW_ON_START,
	SW_TIME_STOP,
	SW_TIME_START,
	SW_TIME_ON_START,
}SW_STATUS;

typedef struct _switch_control
{
	KS_CODE_ACTUATOR_STATUS 	Status;
	KS_CODE_CONTROL_COMMAND 	Command;

	uint8_t		nSW_Num;
	uint16_t 	nControl;
	uint16_t	newOPID;
	uint16_t	preOPID;
	uint16_t 	nStatus;
    uint32_t	nOprTime;

    uint8_t		mTMR_1Sec;
    uint32_t	nHoldingTime;
    uint32_t	nRemainingTime;
    uint32_t	nGetTime;

    SW_STATUS	SWStatus;
	SW_COMMAND	SWCmd;

}SWITCH_CONTROL;

//=================================================================================
typedef struct _adcbat_info
{
    uint32_t    Adc_Buffer[ADC_BUFFER_SIZE];
    uint32_t    Adc_Value;
    float    	fDCVoltage;

    uint32_t	nWaitTime;
}ADCBAT_INFO;
//=================================================================================
typedef enum
{
    SETTING_MODE,
	RUNNING_MODE,
}MODE;

typedef struct _config_data
{
	uint16_t	nNode485Baud;
	uint16_t	nAct485Baud;
}CONFIG_DATA;

//=========================================================================
extern UART_HandleTypeDef		huart1;
extern UART_HandleTypeDef		huart2;
extern UART_HandleTypeDef		huart3;
extern UART_HandleTypeDef 		huart4;

extern UART_RING_BUFFER 		Master_Ring_Buffer;   	// UART1
extern UART_RING_BUFFER 		Network_Ring_Buffer;  	// UART2   마스터 네투웍
extern ACT_RING_BUFFER  		Actuator_Ring_Buffer;   // UART3   릴레이모듈용
extern UART_RING_BUFFER 		Debug_Ring_Buffer;    	// UART4   디버그용

extern ACTNODE_CONTROL			ActNode;
extern ACTUATOR_INSTALLED		Actuator;
extern ACTUATOR_OCS_CONTROL 	Actuator_OCS_Control;
extern ACTUATOR_OCS_EXT_CONTROL Actuator_OCS_EXT_Control;
extern ACTUATOR_SW_CONTROL 		Actuator_SW_Control;

extern CONFIG_DATA				Default_Config;
extern CONFIG_DATA				Install_Config;
extern CONFIG_DATA				Operate_Config;

extern ADCBAT_INFO 				AdcVDC;

extern TIME_FLAG				T3Timer;
extern TIME_FLAG				T6Timer;
extern SEC_TIMER_T 				sec_timer[SEC_MAX_TIMER];
extern MSEC_TIMER_T 			msec_timer[SEC_MAX_TIMER];

extern OCSWITCH_CONTROL			OCS_Control[OCS_MAX_COUNT];
extern SWITCH_CONTROL			SW_Control[SW_MAX_COUNT];
//========================================================================

#endif 
