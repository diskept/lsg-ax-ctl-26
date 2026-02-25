/*
 *  구동기노드 메인.c
 *  마지막수정일 2024.08. 20  GreenHouseActNode_ICTv3
 *  맥노트북에서 전송됨
 *
 *
 *
 *  */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "define.h"
#include "struct.h"
#include "../../Device/Actuator/Actuator_Process.h"
#include "../../Device/Master/Master_Process.h"
#include "../../Device/EEProm/eeprom.h"
#include "tick_timer.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */
/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
UART_RING_BUFFER Master_Ring_Buffer;   // UART1
UART_RING_BUFFER Network_Ring_Buffer;  // UART2
ACT_RING_BUFFER  Actuator_Ring_Buffer;  // UART3
UART_RING_BUFFER Debug_Ring_Buffer;    // UART4
TIME_FLAG		T3Timer;
SEC_TIMER_T 	sec_timer[SEC_MAX_TIMER];
MSEC_TIMER_T 	msec_timer[SEC_MAX_TIMER];

ACTUATOR_INSTALLED			Actuator;
ACTUATOR_OCS_CONTROL 		Actuator_OCS_Control;
ACTUATOR_OCS_EXT_CONTROL 	Actuator_OCS_EXT_Control;

OCSWITCH_CONTROL		OCS_Control[OCS_MAX_COUNT];
ACTUATOR_SW_CONTROL 	Actuator_SW_Control;
SWITCH_CONTROL			SW_Control[SW_MAX_COUNT];

ACTNODE_CONTROL	ActNode;
ADCBAT_INFO 	AdcVDC;

CONFIG_DATA		Default_Config;
CONFIG_DATA		Install_Config;
CONFIG_DATA		Operate_Config;

uint16_t	EPrwData = NODE_CONFIG_NONE;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int _write(int file, char* p, int len)
{
     HAL_UART_Transmit(&huart4, (uint8_t *)p, len, 200);
     return len;
}
/* USER CODE END 0 */

uint16_t Auto_Sw_Check(void)    // 자동 수동 스위치 입력 부분
{
	uint16_t Auto_Sw_Status = 1;
	Auto_Sw_Status = (Auto_SW) ? 3 : 2; // gpioB PIN_2   3:수동 2: 자동
	return Auto_Sw_Status;
}

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();

  MX_TIM3_Init();
  MX_TIM6_Init();

  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_USART4_UART_Init();
  MX_USART3_UART_Init();

  MX_DMA_Init();
  MX_ADC_Init();

  /* USER CODE BEGIN 2 */
	HAL_UART_Receive_IT(&huart1, &Master_Ring_Buffer.RxData, 1);
	HAL_UART_Receive_IT(&huart2, &Network_Ring_Buffer.RxData, 1);
	HAL_UART_Receive_IT(&huart3, &Actuator_Ring_Buffer.RxData, 1);
	HAL_UART_Receive_IT(&huart4, &Debug_Ring_Buffer.RxData, 1);

	HAL_ADC_Start_DMA(&hadc, AdcVDC.Adc_Buffer, 10);

	Debug_printf("==============================================\r\n");
	Debug_printf( 	"Smart GreenHouse Actuator Node System Start!!!\r\n");
	Debug_printf("==============================================\r\n");

	EPrwData = NODE_CONFIG_NONE;

	E2P_Create();
	EPrwData= E2P_Read_Byte(CONFIG_CHECK_ADDR);
	ActNode.nNodeID = Actuator_Get_NodeID();

	switch(EPrwData)
	{
	  case NODE_CONFIG_NONE :
		  Default_Config.nNode485Baud = DEFAULT_NODE_BAUD;
		  Default_Config.nAct485Baud = DEFAULT_ACTUATOR_BAUD;

		  E2P_Erase();
		  E2P_Default_Config_Save(&Default_Config);
		  E2P_Write_Byte(CONFIG_CHECK_ADDR, DEFAULT_WRITE_OK);

		  memcpy(&Operate_Config, &Default_Config, sizeof(CONFIG_DATA));

		  Debug_printf(" NODE_CONFIG_NONE\r\n");
		  Debug_printf(" %04X %04X\r\n",
				  Operate_Config.nNode485Baud, Operate_Config.nAct485Baud);

		  Master_ActNode_Baud_Change(Operate_Config.nNode485Baud);
		  Master_Actuator_Baud_Change(Operate_Config.nAct485Baud);
		  break;

	  case DEFAULT_WRITE_OK :
		  E2P_Default_Config_Load(&Default_Config);
		  memcpy(&Operate_Config, &Default_Config, sizeof(CONFIG_DATA));

		  Debug_printf(" DEFAULT_WRITE_OK\r\n");
		  Debug_printf(" %04X %04X\r\n",
				  Operate_Config.nNode485Baud, Operate_Config.nAct485Baud);

		  Master_ActNode_Baud_Change(Operate_Config.nNode485Baud);
		  Master_Actuator_Baud_Change(Operate_Config.nAct485Baud);
		  break;

	  case INSTALL_WRITE_OK :
		  E2P_Install_Config_Load(&Install_Config);
		  memcpy(&Operate_Config, &Install_Config, sizeof(CONFIG_DATA));

		  Debug_printf(" INSTALL_WRITE_OK\r\n");
		  Debug_printf(" %04X %04X\r\n",
				  Operate_Config.nNode485Baud, Operate_Config.nAct485Baud);

		  Master_ActNode_Baud_Change(Operate_Config.nNode485Baud);
		  Master_Actuator_Baud_Change(Operate_Config.nAct485Baud);
		  break;
	}

	Master_NodeData_Initialize();
	Actuator_Install_Initialize();

	if (HAL_TIM_Base_Start_IT(&htim3) != HAL_OK)
	{
	  Error_Handler();
	}

	if (HAL_TIM_Base_Start_IT(&htim6) != HAL_OK)
	{
	  Error_Handler();
	}

	NETWORK_POWER_ON;
	Debug_printf(" ActNode.nNodeID = %04X\r\n" ,ActNode.nNodeID);

	Actuator_OCS_Control.Step = OCS_RELAY_STEP_IDLE;
	Actuator_OCS_EXT_Control.Step = OCS_EXT_RELAY_STEP_IDLE;
	Actuator_SW_Control.Step = SW_RELAY_STEP_IDLE;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
//	int a = 1;

	while (1)
	{
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
		Master_Buffer_Process();
		Actuator_Buffer_Process();

		ActNode.NodeReg[CON_ACTNODE_CMD_ADDR] = Auto_Sw_Check(); // add by jins  501번지에 1,2,3 중 하나

		Actuator_OCS_Relay_Control_Proecess();
		if(NodeVersion == RELAY_16CH_VER)
		{
			Actuator_OCS_EXT_Relay_Control_Proecess();
		}
		Actuator_SW_Relay_Control_Proecess();

//		if(a >= 50000)
//		{
//			ActuatorNode_NetworkSend_Process(0x00);
//			a = 1;
//		}
//		else
//		{
//			a++;
//		}
	}
	/* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

	/** Initializes the RCC Oscillators according to the specified parameters
	* in the RCC_OscInitTypeDef structure.
	*/
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI14|RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSI14State = RCC_HSI14_ON;
	RCC_OscInitStruct.HSI14CalibrationValue = 16;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
	RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
	Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	*/
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
							  |RCC_CLOCKTYPE_PCLK1;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
	{
	Error_Handler();
	}

	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
	PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;

	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
	{
		Error_Handler();
	}
}

/* USER CODE BEGIN 4 */
char str[20] = "Test 485 Line\n";

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if(GPIO_Pin == GPIO_PIN_2)
  {
	  Debug_printf(" Button Press\n");
//	  ActNode.NodeReg[ACTNODE_STATUS_ADDR] =  4;
  }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM3)                        	// 1msec
	{
		T3Timer.nTCount_1ms++;

		if(T3Timer.nTCount_1ms % 500 == 0)
		{
			HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_15);
		}

		Master_1ms_Process();
		Actuator_1ms_Process();
	}
	else if (htim->Instance == TIM6)                    // 1sec
	{
		if( ActNode.nNodeID != Actuator_Get_NodeID() ) {
			ActNode.nNodeID = Actuator_Get_NodeID();

			NVIC_SystemReset();
		}

		sec_timer_inc_tick();
		Master_Actuator_OnTime_Process();
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == huart1.Instance)
    {
    	HAL_UART_Receive_IT(&huart1, &Master_Ring_Buffer.RxData, 1);
    	Master_RS485_Push_Data(Master_Ring_Buffer.RxData);

    	ActNode.nRxEndTime = RX_END_TIME;
    }
    else if (huart->Instance == huart2.Instance)
    {
    	HAL_UART_Receive_IT(&huart2, &Network_Ring_Buffer.RxData, 1);
    }
    else if (huart->Instance == huart3.Instance)
    {

    	HAL_UART_Receive_IT(&huart3, &Actuator_Ring_Buffer.RxData, 1);
    	Actuator_RS485_Push_Data(Actuator_Ring_Buffer.RxData);
    }
    else if (huart->Instance == huart4.Instance)
    {
    	HAL_UART_Receive_IT(&huart4, &Debug_Ring_Buffer.RxData, 1);
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
