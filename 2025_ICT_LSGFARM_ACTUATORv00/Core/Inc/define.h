///-------------------------------------------------------------
///
///
///-------------------------------------------------------------

#ifndef __DEFINE_H
#define __DEFINE_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal.h"
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "main.h"

// Relay Type Version
#define Rly_Old							1								// old version
#define Rly_New							2								// new Version

#define RelayVersion   					Rly_New


// KSX 국가표준검정 관련 Define
#define KSX_TEST            			1   							// KSX 국가표준검정 지원 여부

// Relay Version 정의
#define RELAY_16CH_VER      			2								// 16채널 릴레이 보드
#define RELAY_32CH_VER      			3   							// 32채널 릴레이 보드

// KS X 3267 기본 구성 스위치 16ch, 개폐기 8ch(relay 16)
// Actuator node Ver 1.0 : 스위치 16CH, 개페기 8ch(relay 16) + Ext. 개페기 8ch(relay 16)
// Actuator node Ver 1.1 : 스위치 16CH, 개폐기 16ch(relay 32) + Ext. 개폐기 16ch(relay 32)
#define NodeVersion						RELAY_32CH_VER					// OSC : 1 = 릴레이모듈(8CH) 1버전, 2 = 릴레이모듈(16CH) 2버전, 3 = 릴레이모듈(32CH)   이외는 버전 1


#define SETBIT(GPIO,BIT)				(GPIO->BSRR = BIT)
#define CLEARBIT(GPIO,BIT)				(GPIO->BRR = BIT)
#define TOGGLEBIT(GPIO, BIT) 			(GPIO->ODR ^= BIT)
#define TESTBIT(GPIOx, PinNum)   		((GPIOx->IDR & PinNum) == 0 ? 0 : 1)

#define BITisSET(value, pos) 			(value |= (1U<< pos))
#define BITisCLEAR(value, pos) 			(value &= (~(1U<< pos)))


#define UART_RX_RING_BUFFER_SIZE   		1024   						// 링버퍼수신 값 초기 256 ---> 512
#define UART_TX_RING_BUFFER_SIZE   		1024

#define	ADC_BUFFER_SIZE					10
#define ADC_MINUSOFFSET					0
#define ADC_REF_33_VOLT					3300

#define SEC_MAX_TIMER 					32
#define OCS_MAX_COUNT 					16							//검증 시 8, 16ch 릴레이보드 2개 16, 32초 릴레이보드 1개 16
#define SW_MAX_COUNT 					16

#define CR								0x0D
#define LF								0x0A

#define STX								0x02
#define ETX								0x03
#define EOT								0x04

#define TRUE 							1
#define FALSE 							0

//================================================================================================
#define GHC_TX_Enable 					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET );
#define GHC_RX_Enable 					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET );

#define ACT_TX_Enable 					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET );
#define ACT_RX_Enable 					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET );

#define	SYS_LED_ON						HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET );  	// LED2
#define	SYS_LED_OFF						HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET );

#define	ACT_LED_ON						HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET );   	// LED6
#define	ACT_LED_OFF						HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET );

#define	MST_LED_ON						HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET );  	// LED7
#define	MST_LED_OFF						HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_SET );

#define	SEN_12V_POWER_ON				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET );   		// 12V Sensor Power On
#define	SEN_12V_POWER_OFF				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET ); 		// 12V Sensor Power Off

#define	SEN_5V_POWER_ON					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET );   		// 5V Sensor Power On
#define	SEN_5V_POWER_OFF				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET ); 		// 5V Sensor Power Off

#define	NETWORK_POWER_ON				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET );   	// Network Power On
#define	NETWORK_POWER_OFF				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET ); 	// Network Power Off

#define Auto_SW         				TESTBIT(GPIOB, GPIO_PIN_2)  								// add by jins 2024-01-25  자동수동 스위치

#define NODE_ID1						TESTBIT(GPIOB, GPIO_PIN_3)
#define NODE_ID2						TESTBIT(GPIOB, GPIO_PIN_4)
#define NODE_ID3						TESTBIT(GPIOB, GPIO_PIN_5)
#define NODE_ID4						TESTBIT(GPIOB, GPIO_PIN_13)
#define NODE_ID5						TESTBIT(GPIOB, GPIO_PIN_14)
#define SETTING							TESTBIT(GPIOB, GPIO_PIN_15)

#define WSR_09R							TESTBIT(GPIOB, GPIO_PIN_6)

#define POWER_ON_DELAY					5000
#define POWER_RST_DELAY					1000

#define ACT_SCAN_DELAY					100
#define ACT_SKIP_DELAY					500
#define ACT_RETRY_CNT					10
#define ACT_SCAN_INTERVAL				500

//#define SENSOR_SCAN_TIME	1000
#define MASTER_RELOAD_TIME				1000
#define	TACT_SWTIME						3000
#define RX_CHECK_TIME					1000
#define RX_END_TIME						4

//==================================================================================================
#define	MASTER_DEBUG					0
#define	ACTUATOR_DEBUG					0
#define	NETWORK_OUTPUT					1

//==================================================================================================
//Actuator Node Info Address
#define	INSTITUTION_CODE				1
#define	COMPANY_CODE					2
#define	PRODUCT_TYPE					3
#define	PRODUCT_CODE					4
#define	PROTOCOL_VER					5
#define	CHANNEL_CNT						6
#define	SERIAL_NUM						7

//Setup Actuator Info Address
#define SWITCH_1_ADDR					101
#define SWITCH_2_ADDR					102
#define SWITCH_3_ADDR					103
#define SWITCH_4_ADDR					104
#define SWITCH_5_ADDR					105
#define SWITCH_6_ADDR					106
#define SWITCH_7_ADDR					107
#define SWITCH_8_ADDR					108
#define SWITCH_9_ADDR					109
#define SWITCH_10_ADDR					110
#define SWITCH_11_ADDR					111
#define SWITCH_12_ADDR					112
#define SWITCH_13_ADDR					113
#define SWITCH_14_ADDR					114
#define SWITCH_15_ADDR					115
#define SWITCH_16_ADDR					116

#define OCSWITCH_1_ADDR					117
#define OCSWITCH_2_ADDR					118
#define OCSWITCH_3_ADDR					119
#define OCSWITCH_4_ADDR					120
#define OCSWITCH_5_ADDR					121
#define OCSWITCH_6_ADDR					122
#define OCSWITCH_7_ADDR					123
#define OCSWITCH_8_ADDR					124
//=========================================== Adding
#define OCSWITCH_9_ADDR					125
#define OCSWITCH_10_ADDR				126
#define OCSWITCH_11_ADDR				127
#define OCSWITCH_12_ADDR				128
#define OCSWITCH_13_ADDR				129
#define OCSWITCH_14_ADDR				130
#define OCSWITCH_15_ADDR				131
#define OCSWITCH_16_ADDR				132

//Actuator Node Status Address
#define ACTNODE_OPID_0_ADDR				201
#define ACTNODE_STATUS_ADDR				202

//Actuator Status Address
#define SWITCH_1_OPID_1_ADDR			203
#define SWITCH_1_STATUS_ADDR			204
#define SWITCH_1_TIME_ADDR				205

#define SWITCH_2_OPID_2_ADDR			207
#define SWITCH_2_STATUS_ADDR			208
#define SWITCH_2_TIME_ADDR				209

#define SWITCH_3_OPID_3_ADDR			211
#define SWITCH_3_STATUS_ADDR			212
#define SWITCH_3_TIME_ADDR				213

#define SWITCH_4_OPID_4_ADDR			215
#define SWITCH_4_STATUS_ADDR			216
#define SWITCH_4_TIME_ADDR				217

#define SWITCH_5_OPID_5_ADDR			219
#define SWITCH_5_STATUS_ADDR			220
#define SWITCH_5_TIME_ADDR				221

#define SWITCH_6_OPID_6_ADDR			223
#define SWITCH_6_STATUS_ADDR			224
#define SWITCH_6_TIME_ADDR				225

#define SWITCH_7_OPID_7_ADDR			227
#define SWITCH_7_STATUS_ADDR			228
#define SWITCH_7_TIME_ADDR				229

#define SWITCH_8_OPID_8_ADDR			231
#define SWITCH_8_STATUS_ADDR			232
#define SWITCH_8_TIME_ADDR				233

#define SWITCH_9_OPID_9_ADDR			235
#define SWITCH_9_STATUS_ADDR			236
#define SWITCH_9_TIME_ADDR				237

#define SWITCH_10_OPID_10_ADDR			239
#define SWITCH_10_STATUS_ADDR			240
#define SWITCH_10_TIME_ADDR				241

#define SWITCH_11_OPID_11_ADDR			243
#define SWITCH_11_STATUS_ADDR			244
#define SWITCH_11_TIME_ADDR				245

#define SWITCH_12_OPID_12_ADDR			247
#define SWITCH_12_STATUS_ADDR			248
#define SWITCH_12_TIME_ADDR				249

#define SWITCH_13_OPID_13_ADDR			251
#define SWITCH_13_STATUS_ADDR			252
#define SWITCH_13_TIME_ADDR				253

#define SWITCH_14_OPID_14_ADDR			255
#define SWITCH_14_STATUS_ADDR			256
#define SWITCH_14_TIME_ADDR				257

#define SWITCH_15_OPID_15_ADDR			259
#define SWITCH_15_STATUS_ADDR			260
#define SWITCH_15_TIME_ADDR				261

#define SWITCH_16_OPID_16_ADDR			263
#define SWITCH_16_STATUS_ADDR			264
#define SWITCH_16_TIME_ADDR				265

//==============================================
#define OCSWITCH_1_OPID_17_ADDR			267
#define OCSWITCH_1_STATUS_ADDR			268
#define OCSWITCH_1_TIME_ADDR			269

#define OCSWITCH_2_OPID_18_ADDR			271
#define OCSWITCH_2_STATUS_ADDR			272
#define OCSWITCH_2_TIME_ADDR			273

#define OCSWITCH_3_OPID_19_ADDR			275
#define OCSWITCH_3_STATUS_ADDR			276
#define OCSWITCH_3_TIME_ADDR			277

#define OCSWITCH_4_OPID_20_ADDR			279
#define OCSWITCH_4_STATUS_ADDR			280
#define OCSWITCH_4_TIME_ADDR			281

#define OCSWITCH_5_OPID_21_ADDR			283
#define OCSWITCH_5_STATUS_ADDR			284
#define OCSWITCH_5_TIME_ADDR			285

#define OCSWITCH_6_OPID_22_ADDR			287
#define OCSWITCH_6_STATUS_ADDR			288
#define OCSWITCH_6_TIME_ADDR			289

#define OCSWITCH_7_OPID_23_ADDR			291
#define OCSWITCH_7_STATUS_ADDR			292
#define OCSWITCH_7_TIME_ADDR			293

#define OCSWITCH_8_OPID_24_ADDR			295
#define OCSWITCH_8_STATUS_ADDR			296
#define OCSWITCH_8_TIME_ADDR			297

//=========================================== Adding
#define OCSWITCH_9_OPID_25_ADDR			299
#define OCSWITCH_9_STATUS_ADDR			300
#define OCSWITCH_9_TIME_ADDR			301

#define OCSWITCH_10_OPID_26_ADDR		303
#define OCSWITCH_10_STATUS_ADDR			304
#define OCSWITCH_10_TIME_ADDR			305

#define OCSWITCH_11_OPID_27_ADDR		307
#define OCSWITCH_11_STATUS_ADDR			308
#define OCSWITCH_11_TIME_ADDR			309

#define OCSWITCH_12_OPID_28_ADDR		311
#define OCSWITCH_12_STATUS_ADDR			312
#define OCSWITCH_12_TIME_ADDR			313

#define OCSWITCH_13_OPID_29_ADDR		315
#define OCSWITCH_13_STATUS_ADDR			316
#define OCSWITCH_13_TIME_ADDR			317

#define OCSWITCH_14_OPID_30_ADDR		319
#define OCSWITCH_14_STATUS_ADDR			320
#define OCSWITCH_14_TIME_ADDR			321

#define OCSWITCH_15_OPID_31_ADDR		323
#define OCSWITCH_15_STATUS_ADDR			324
#define OCSWITCH_15_TIME_ADDR			325

#define OCSWITCH_16_OPID_32_ADDR		327
#define OCSWITCH_16_STATUS_ADDR			328
#define OCSWITCH_16_TIME_ADDR			329

//Actuator Contorl Address
#define CON_ACTNODE_CMD_ADDR			501
#define CON_ACTNODE_OPID_0_ADDR			502

//============================================== SWITCH
#define CON_SWITCH_1_CMD_ADDR			503
#define CON_SWITCH_1_OPID_1_ADDR		504
#define CON_SWITCH_1_TIME_ADDR			505

#define CON_SWITCH_2_CMD_ADDR			507
#define CON_SWITCH_2_OPID_2_ADDR		508
#define CON_SWITCH_2_TIME_ADDR			509

#define CON_SWITCH_3_CMD_ADDR			511
#define CON_SWITCH_3_OPID_3_ADDR		512
#define CON_SWITCH_3_TIME_ADDR			513

#define CON_SWITCH_4_CMD_ADDR			515
#define CON_SWITCH_4_OPID_4_ADDR		516
#define CON_SWITCH_4_TIME_ADDR			517

#define CON_SWITCH_5_CMD_ADDR			519
#define CON_SWITCH_5_OPID_5_ADDR		520
#define CON_SWITCH_5_TIME_ADDR			521

#define CON_SWITCH_6_CMD_ADDR			523
#define CON_SWITCH_6_OPID_6_ADDR		524
#define CON_SWITCH_6_TIME_ADDR			525

#define CON_SWITCH_7_CMD_ADDR			527
#define CON_SWITCH_7_OPID_7_ADDR		528
#define CON_SWITCH_7_TIME_ADDR			529

#define CON_SWITCH_8_CMD_ADDR			531
#define CON_SWITCH_8_OPID_8_ADDR		532
#define CON_SWITCH_8_TIME_ADDR			533

#define CON_SWITCH_9_CMD_ADDR			535
#define CON_SWITCH_9_OPID_9_ADDR		536
#define CON_SWITCH_9_TIME_ADDR			537

#define CON_SWITCH_10_CMD_ADDR			539
#define CON_SWITCH_10_OPID_10_ADDR		540
#define CON_SWITCH_10_TIME_ADDR			541

#define CON_SWITCH_11_CMD_ADDR			543
#define CON_SWITCH_11_OPID_11_ADDR		544
#define CON_SWITCH_11_TIME_ADDR			545

#define CON_SWITCH_12_CMD_ADDR			547
#define CON_SWITCH_12_OPID_12_ADDR		548
#define CON_SWITCH_12_TIME_ADDR			549

#define CON_SWITCH_13_CMD_ADDR			551
#define CON_SWITCH_13_OPID_13_ADDR		552
#define CON_SWITCH_13_TIME_ADDR			553

#define CON_SWITCH_14_CMD_ADDR			555
#define CON_SWITCH_14_OPID_14_ADDR		556
#define CON_SWITCH_14_TIME_ADDR			557

#define CON_SWITCH_15_CMD_ADDR			559
#define CON_SWITCH_15_OPID_15_ADDR		560
#define CON_SWITCH_15_TIME_ADDR			561

#define CON_SWITCH_16_CMD_ADDR			563
#define CON_SWITCH_16_OPID_16_ADDR		564
#define CON_SWITCH_16_TIME_ADDR			565

//============================================== OCSWITCH
#define CON_OCSWITCH_1_CMD_ADDR			567
#define CON_OCSWITCH_1_OPID_17_ADDR		568
#define CON_OCSWITCH_1_TIME_ADDR		569

#define CON_OCSWITCH_2_CMD_ADDR			571
#define CON_OCSWITCH_2_OPID_18_ADDR		572
#define CON_OCSWITCH_2_TIME_ADDR		573

#define CON_OCSWITCH_3_CMD_ADDR			575
#define CON_OCSWITCH_3_OPID_19_ADDR		576
#define CON_OCSWITCH_3_TIME_ADDR		577

#define CON_OCSWITCH_4_CMD_ADDR			579
#define CON_OCSWITCH_4_OPID_20_ADDR		580
#define CON_OCSWITCH_4_TIME_ADDR		581

#define CON_OCSWITCH_5_CMD_ADDR			583
#define CON_OCSWITCH_5_OPID_21_ADDR		584
#define CON_OCSWITCH_5_TIME_ADDR		585

#define CON_OCSWITCH_6_CMD_ADDR			587
#define CON_OCSWITCH_6_OPID_22_ADDR		588
#define CON_OCSWITCH_6_TIME_ADDR		589

#define CON_OCSWITCH_7_CMD_ADDR			591
#define CON_OCSWITCH_7_OPID_23_ADDR		592
#define CON_OCSWITCH_7_TIME_ADDR		593

#define CON_OCSWITCH_8_CMD_ADDR			595
#define CON_OCSWITCH_8_OPID_24_ADDR		596
#define CON_OCSWITCH_8_TIME_ADDR		597

//=========================================== Adding
#define CON_OCSWITCH_9_CMD_ADDR			599
#define CON_OCSWITCH_9_OPID_25_ADDR		600
#define CON_OCSWITCH_9_TIME_ADDR		601

#define CON_OCSWITCH_10_CMD_ADDR		603
#define CON_OCSWITCH_10_OPID_26_ADDR	604
#define CON_OCSWITCH_10_TIME_ADDR		605

#define CON_OCSWITCH_11_CMD_ADDR		607
#define CON_OCSWITCH_11_OPID_27_ADDR	608
#define CON_OCSWITCH_11_TIME_ADDR		609

#define CON_OCSWITCH_12_CMD_ADDR		611
#define CON_OCSWITCH_12_OPID_28_ADDR	612
#define CON_OCSWITCH_12_TIME_ADDR		613

#define CON_OCSWITCH_13_CMD_ADDR		615
#define CON_OCSWITCH_13_OPID_29_ADDR	616
#define CON_OCSWITCH_13_TIME_ADDR		617

#define CON_OCSWITCH_14_CMD_ADDR		619
#define CON_OCSWITCH_14_OPID_30_ADDR	620
#define CON_OCSWITCH_14_TIME_ADDR		621

#define CON_OCSWITCH_15_CMD_ADDR		623
#define CON_OCSWITCH_15_OPID_31_ADDR	624
#define CON_OCSWITCH_15_TIME_ADDR		625

#define CON_OCSWITCH_16_CMD_ADDR		627
#define CON_OCSWITCH_16_OPID_32_ADDR	628
#define CON_OCSWITCH_16_TIME_ADDR		629

//===============================================================================================
#define MODBUS_READ_HOLDING_COMMAND			0x03
#define MODBUS_READ_INPUT_COMMAND			0x04
#define MODBUS_WRITE_SINGLE_COMMAND			0x06
#define MODBUS_WRITE_MULTIPLE_COMMAND		0x10
#define MODBUS_WRITE_MULTIPLE_COIL_COMMAND	0x0F
#define MODBUS_WRITE_SINGLE_COIL_COMMAND	0x05

#define MODBUSID_MIN					1
#define MODBUSID_MAX					31

#define DEFAULT_REGISTER_MAX			631
#define DEFAULT_NODE_BAUD				0x0001
#define DEFAULT_SENSOR_BAUD				0x0001
#define DEFAULT_ACTUATOR_BAUD			0x0003
#define DEFAULT_GHA_INSTALL_CODE		0x00A1
#define DEFAULT_GHB_INSTALL_CODE		0x00B1
#define DEFAULT_LB_INSTALL_CODE			0x00C1
#define DEFAULT_ACT_INSTALL_CODE		0x00D1
#define DEFAULT_ACT_CUSTOM_CODE			0x00E1

#define	NODE_CONFIG_NONE			0xFFFF
#define	DEFAULT_WRITE_OK			0xFF00
#define	INSTALL_WRITE_OK			0x00FF

#define CONFIG_CHECK_ADDR      		0x0010
#define DEFAULT_CONFIG_ADDR			0x0100
#define INSTALL_CONFIG_ADDR			0x0110

#define NODE_COMMAND_ADDR			0xF000
#define	NODE_NODE_BAUD_ADDR			0xF001
#define NODE_SENSOR_BAUD_ADDR		0xF002
#define NODE_CODE_ADDR				0xF003

#endif 
