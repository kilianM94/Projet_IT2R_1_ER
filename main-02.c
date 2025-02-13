/**
  ******************************************************************************
  * @file    Templates/Src/main.c 
  * @author  MCD Application Team
  * @brief   STM32F4xx HAL API Template project 
  *
  * @note    modified by ARM
  *          The modifications allow to use this file as User Code Template
  *          within the Device Family Pack.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"                   // ARM::CMSIS:RTOS:Keil RTX

#include "Driver_USART.h"               // ::CMSIS Driver:USART
#include "Driver_CAN.h"                 // ::CMSIS Driver:CAN
#include "stm32f4xx.h"                  // Device header
#include "Board_LED.h"                  // ::Board Support:LED

#ifdef _RTE_
#include "RTE_Components.h"             // Component selection
#endif
#ifdef RTE_CMSIS_RTOS2                  // when RTE component CMSIS RTOS2 is used
#include "cmsis_os2.h"  // ::CMSIS:RTOS2
#include <stdio.h>
#include <string.h>
#endif



#ifdef RTE_CMSIS_RTOS2_RTX5
/**
  * Override default HAL_GetTick function
  */
uint32_t HAL_GetTick (void) {
  static uint32_t ticks = 0U;
         uint32_t i;

  if (osKernelGetState () == osKernelRunning) {
    return ((uint32_t)osKernelGetTickCount ());
  }

  /* If Kernel is not running wait approximately 1 ms then increment 
     and return auxiliary tick counter value */
  for (i = (SystemCoreClock >> 14U); i > 0U; i--) {
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
    __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
  }
  return ++ticks;
}

#endif

/** @addtogroup STM32F4xx_HAL_Examples
  * @{
  */

/** @addtogroup Templates
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define ID_Capteur_US			0x008
#define ID_GPS_latitude		0x040
#define ID_GPS_longitude	0x042
#define ID_GPS_horodatage	0x044
#define ID_Moto_Dir		    0x400
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void Error_Handler(void);
void CallbackUART2(uint32_t event);
void CallbackUART3 (uint32_t event);
osMailQId  ID_BAL ;
osMailQDef (NomBAL, 1, char) ;
void myCAN2_callback(uint32_t obj_idx, uint32_t event);
void CANthreadT(void const *argument);
void CANthreadR(void const *argument);
extern ARM_DRIVER_USART Driver_USART2;
extern ARM_DRIVER_USART Driver_USART3;
int strncmp (const char*,const char*, unsigned int);

extern ARM_DRIVER_CAN Driver_CAN2;
void TacheModule(void const*argument);
void TacheRFID(void const*argument);
void ClearBuffer(void const*argument);
osThreadId ID_TacheModule;
osThreadId ID_ClearBuffer;
osThreadId ID_TacheRFID;
osThreadId  ID_CANthreadT;
osThreadId  ID_CANthreadR;
osThreadDef(TacheModule,osPriorityNormal,1,0);
osThreadDef(TacheRFID,osPriorityNormal,1,0);
osThreadDef(ClearBuffer,osPriorityNormal,1,0);

void USART2_INIT(void)
	{
	Driver_USART2.Initialize(CallbackUART2);
	Driver_USART2.PowerControl(ARM_POWER_FULL);
	Driver_USART2.Control(ARM_USART_MODE_ASYNCHRONOUS|
												ARM_USART_DATA_BITS_8|
												ARM_USART_STOP_BITS_1|
												ARM_USART_PARITY_NONE|
												ARM_USART_FLOW_CONTROL_NONE,
												9600);
	
	Driver_USART2.Control(ARM_USART_CONTROL_TX,1);
	Driver_USART2.Control(ARM_USART_CONTROL_RX,1);
	}
	void USART3_INIT(void)
	{
		Driver_USART3.Initialize(CallbackUART3);
	Driver_USART3.PowerControl(ARM_POWER_FULL);
	Driver_USART3.Control(ARM_USART_MODE_ASYNCHRONOUS|
												ARM_USART_DATA_BITS_8|
												ARM_USART_STOP_BITS_1|
												ARM_USART_PARITY_NONE|
												ARM_USART_FLOW_CONTROL_NONE,
												9600);
	
	Driver_USART3.Control(ARM_USART_CONTROL_TX,1);
	Driver_USART3.Control(ARM_USART_CONTROL_RX,1);
	}
	
void InitCAN2(){
	Driver_CAN2.Initialize(NULL,myCAN2_callback);
	Driver_CAN2.PowerControl(ARM_POWER_FULL);
	Driver_CAN2.SetMode(ARM_CAN_MODE_INITIALIZATION);
	Driver_CAN2.SetBitrate(ARM_CAN_BITRATE_NOMINAL,    // d�bit fixe
												 125000,                     // 125 kbits/s (LS)
												 ARM_CAN_BIT_PROP_SEG(5U)   |   //  prop. seg = 5 TQ
												 ARM_CAN_BIT_PHASE_SEG1(1U) | // phase seg1 = 1 TQ
												 ARM_CAN_BIT_PHASE_SEG2(1U) | // phase seg2 = 1 TQ
												 ARM_CAN_BIT_SJW(1U));          // Resync. Seg = 1 TQ
	
	Driver_CAN2.ObjectConfigure(2,ARM_CAN_OBJ_TX); // Objet 2 pour �mission
	Driver_CAN2.ObjectConfigure(0,ARM_CAN_OBJ_RX); // Objet 0 pour r�ception
	Driver_CAN2.SetMode(ARM_CAN_MODE_NORMAL); // fin initialisation
	Driver_CAN2.ObjectSetFilter(2, ARM_CAN_FILTER_ID_EXACT_ADD ,ARM_CAN_STANDARD_ID(ID_Capteur_US),0);
	Driver_CAN2.Initialize(NULL,myCAN2_callback);
}

osThreadDef (CANthreadT, osPriorityNormal, 1, 0);
osThreadDef (CANthreadR, osPriorityNormal, 1, 0);


/* Private functions ---------------------------------------------------------*/
/**
	
	
  * @brief  Main program
  * @param  None
  * @retval None
  */

	
	void CallbackUART2 (uint32_t event)
	{
		
		if (event == ARM_USART_EVENT_RECEIVE_COMPLETE )
		{
		osSignalSet(ID_TacheRFID,0x01);
    }
		if (event == ARM_USART_EVENT_RX_OVERFLOW)
		{
			osSignalSet(ID_ClearBuffer,0x01);
		}
		
}
		void CallbackUART3 (uint32_t event)
	{
		if (event == ARM_USART_EVENT_SEND_COMPLETE )
		{
		osSignalSet(ID_TacheModule,0x01);
	}

		
}
void myCAN2_callback(uint32_t obj_idx, uint32_t event)// arguments impos�s
{
		switch (event)
		{
			case ARM_CAN_EVENT_SEND_COMPLETE:  
				osSignalSet(ID_CANthreadT, 0x01);
				break;
			case ARM_CAN_EVENT_RECEIVE:       
				osSignalSet(ID_CANthreadR, 0x01);
				break;
		}
}

	int main(void)
{

	
  /* STM32F4xx HAL library initialization:
       - Configure the Flash prefetch, Flash preread and Buffer caches
       - Systick timer is configured by default as source of time base, but user 
             can eventually implement his proper time base source (a general purpose 
             timer for example or other time source), keeping in mind that Time base 
             duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and 
             handled in milliseconds basis.
       - Low Level Initialization
     */
  HAL_Init();

  /* Configure the system clock to 168 MHz */
  SystemClock_Config();
  SystemCoreClockUpdate();

  /* Add your application code here
     */
	//#ifdef RTE_CMSIS_RTOS2
  /* Initialize CMSIS-RTOS2 */
  osKernelInitialize ();
	
	//NVIC_SetPriority(USART3_IRQn,2);		// n�cessaire ? (si LCD ?)
	
	LED_Initialize ();
	USART2_INIT();
	USART3_INIT();
	InitCAN2();	
	

  /* Create thread functions that start executing, 
  Example: osThreadNew(app_main, NULL, NULL); */
	ID_BAL = osMailCreate(osMailQ(NomBAL), NULL); 
	ID_TacheModule = osThreadCreate(osThread(TacheModule),NULL);
	ID_TacheRFID = osThreadCreate(osThread(TacheRFID),NULL);
	ID_ClearBuffer = osThreadCreate(osThread(ClearBuffer),NULL);
		ID_CANthreadT=osThreadCreate (osThread (CANthreadT), NULL);
	  ID_CANthreadR=osThreadCreate (osThread (CANthreadR), NULL);

	/* Start thread execution */
  osKernelStart();
	
	//LED_On (3);
//#endif
	osDelay(osWaitForever);
	

}
void TacheRFID(void const*argument)
{
	char ID[13]="0D0093641BE1"; // id du bon badge rfid
	char ID2[13]="08008C23E94E";
	char tab[14],info_play[10],info_track[10], info_volume[10]; // d�claration des variables 
	char *ptr;

  /* Infinite loop */
  while (1)
  {
	
			
		Driver_USART2.Receive(tab,14);
		osSignalWait(0x01,osWaitForever);
		
	  
		tab[13]=0;
		LED_Off           (3);
      if ((strcmp (ID,tab+1)==0) && (tab[0]==0x02))
			{
				
				ptr = osMailAlloc(ID_BAL, osWaitForever);
				*ptr = 0x01;// valeur � envoyer 
				osMailPut(ID_BAL, ptr);                         
			LED_On           (1);
			LED_Off          (2);

			}
				else if ((strcmp (ID2,tab+1))==0 && (tab[0]==0x02))
			{
						ptr = osMailAlloc(ID_BAL, osWaitForever);
				*ptr = 0x02;// valeur � envoyer 
				osMailPut(ID_BAL, ptr); 
			LED_Off          (1);
			LED_On           (2);

			}
		
  }
}
void TacheModule(void const* argument)
{
	char info_play[10],info_volume[10],info_track[10],info_track2[10],tabtrack2[10],track=0x01;
	
	osEvent EVretour;
char *recep, valeur_recue;
short checksum_play, checksum_track,checksum_track2,checksum_volume;


	
  info_play[0]=0x7E;
	info_play[1]=0xFF;
	info_play[2]=0x06;
	info_play[3]=0x0B;
	info_play[4]=0x00;
	info_play[5]=0x00;
	info_play[6]=0x00;
	checksum_play = 0x0000 - info_play[1]-info_play[2]-info_play[3]-info_play[4]-info_play[5]-info_play[6];
	info_play[7]=(checksum_play>>16)-0x0001;
	info_play[8]= checksum_play;
	info_play[9]=0xEF;

			  info_track[0]=0x7E;
	info_track[1]=0xFF;
	info_track[2]=0x06;
	info_track[3]=0x03;
	info_track[4]=0x00;
	info_track[5]=0x00;
	info_track[6]=0x04;
	checksum_track = 0x0000 - info_track[1]-info_track[2]-info_track[3]-info_track[4]-info_track[5]-info_track[6];
	info_track[7]=(checksum_track>>16)-0x0001;
	info_track[8]= checksum_track;
	info_track[9]=0xEF;

	
	  info_track2[0]=0x7E;
	info_track2[1]=0xFF;
	info_track2[2]=0x06;
	info_track2[3]=0x03;
	info_track2[4]=0x00;
	info_track2[5]=0x00;
	info_track2[6]=0x05;
	checksum_track2 = 0x0000 - info_track2[1]-info_track2[2]-info_track2[3]-info_track2[4]-info_track2[5]-info_track2[6];
	info_track2[7]=(checksum_track2>>16)-0x0001;
	info_track2[8]= checksum_track2;
	info_track2[9]=0xEF;

		info_volume[0]=0x7E;
	info_volume[1]=0xFF;
	info_volume[2]=0x06;
	info_volume[3]=0x06;
	info_volume[4]=0x00;
	info_volume[5]=0x00;
	info_volume[6]=0x10;
	checksum_volume = 0x0000 - info_volume[1]-info_volume[2]-info_volume[3]-info_volume[4]-info_volume[5]-info_volume[6];
	info_volume[7]=(checksum_volume>>16)-0x0001;
	info_volume[8]= checksum_volume;
	info_volume[9]=0xEF;
	

		
	Driver_USART3.Send(info_volume,10);
	osSignalWait(0x01,osWaitForever);

	Driver_USART3.Send(info_play,10);
	osSignalWait(0x01,osWaitForever);


  /* Infinite loop */
  while (1)
  {
LED_Off(1);
			LED_Off(2);
		LED_Off(3);
		EVretour = osMailGet(ID_BAL, osWaitForever);        // attente mail
		recep = EVretour.value.p;// on r�cup�re le pointeur...
		valeur_recue = *recep ;// ...et la valeur point�e
		osMailFree(ID_BAL, recep);                    // lib�ration m�moire allou�e
			 if (valeur_recue==0x01)
			{
							
			
				LED_On(1);
				Driver_USART3.Send(info_track,10);
				osSignalWait(0x01,osWaitForever);
				osDelay(5000);

			}
			else if (valeur_recue==0x02)
			{
		
					LED_On(2);
				Driver_USART3.Send(info_track2,10);
				osSignalWait(0x01,osWaitForever);
				LED_On(3);
				osDelay(5000);

			}

		}
	}
void ClearBuffer(void const*argument)
{
	uint8_t poubelle[200];
	while(1){
	   Driver_USART2.Receive(poubelle,200);
		osSignalWait(0x01,osWaitForever);
		
	}
	
	
	
}
void CANthreadT(void const *argument)
{
	ARM_CAN_MSG_INFO                tx_msg_info;
	char data_buf[8], *ptr;
	int i;
	
	while (1) 
		{
		tx_msg_info.id  = ARM_CAN_STANDARD_ID(ID_Capteur_US	);           // ID CAN exemple
		tx_msg_info.rtr = 0;// 0 = trame DATA
		for(i = 0; i < 8; i++)data_buf[i]=0;
		data_buf[0] = 0xAA;// data � envoyer exemple
		Driver_CAN2.MessageSend(2, &tx_msg_info, data_buf, 1); // envoi
		/* Sommeil sur fin envoi */
		osSignalWait(0x01, osWaitForever);
		osDelay(100);
		}
}
void CANthreadR(void const *argument)
{
		ARM_CAN_MSG_INFO                rx_msg_info;
		char data_buf[8];
		char nb_data, retour;
		int identifiant;
		osEvent evt;// si besoin de gestion du retour (ex : Timeout)...
		while (1) 
			{
				evt = osSignalWait(0x01, osWaitForever);// sommeil sur attente r�ception
				Driver_CAN2.MessageRead(0, &rx_msg_info, data_buf, 8);// 8 data max
				// on traite la trame re�ue
				nb_data = rx_msg_info.dlc ;// r�cup�ration nb_data
				identifiant = rx_msg_info.id;
        retour = data_buf[0];
			}
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 168000000
  *            HCLK(Hz)                       = 168000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 8000000
  *            PLL_M                          = 25
  *            PLL_N                          = 336
  *            PLL_P                          = 2
  *            PLL_Q                          = 7
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 5
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();

  /* The voltage scaling allows optimizing the power consumption when the device is 
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;  
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  /* STM32F405x/407x/415x/417x Revision Z devices: prefetch is supported */
  if (HAL_GetREVID() == 0x1001)
  {
    /* Enable the Flash prefetch */
    __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  /* User may add here some code to deal with this error */
  while(1)
  {
  }
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}

#endif

/**
  * @}
  */ 

/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/