/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "NanoEdgeAI.h"
#include "dht11_timeout.h"
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
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

// PV pour l'entrainement

enum neai_state state;                 // Pour capturer le retour des fonctions NEAI
bool use_pretrained = false;            // false = apprentissage depuis zéro sur la carte
uint8_t similarity = 90;                    // Résultat du detect() : 0-100%
float input_signal [2];
uint8_t learn_count = 0;
uint8_t acquisition_ok;

// PV pour la détection

float temperature, humidity;

// --- NOUVEAU : Variables pour réception UART par interruption ---

#define RX_BUFFER_SIZE 20
uint8_t rxBuffer[RX_BUFFER_SIZE];
uint8_t rxIndex = 0;
volatile bool commandeLedRecue = false;
uint8_t ledNumero = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// --- Callback d'interruption UART ---
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        if (rxBuffer[rxIndex] == '\n' || rxBuffer[rxIndex] == '\r')
        {
            rxBuffer[rxIndex] = '\0';

            // LED individuelles 1-4
            if (strncmp((char*)rxBuffer, "LED1:", 5) == 0) ledNumero = 1;
            else if (strncmp((char*)rxBuffer, "LED2:", 5) == 0) ledNumero = 2;
            else if (strncmp((char*)rxBuffer, "LED3:", 5) == 0) ledNumero = 3;
            else if (strncmp((char*)rxBuffer, "LED4:", 5) == 0) ledNumero = 4;
            // LED all (allume/éteint tout)
            else if (strncmp((char*)rxBuffer, "LEDall:", 7) == 0) ledNumero = 0; // 0 = spécial

            if (ledNumero != 0 || strncmp((char*)rxBuffer, "LEDall:", 7) == 0) {
                commandeLedRecue = true;
            }

            rxIndex = 0;
            memset(rxBuffer, 0, RX_BUFFER_SIZE);
        }
        else
        {
            rxIndex++;
            if (rxIndex >= RX_BUFFER_SIZE) rxIndex = 0;
        }

        HAL_UART_Receive_IT(&huart2, &rxBuffer[rxIndex], 1);
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	     // Désactive l'interruption externe sur la ligne 0
	     HAL_NVIC_DisableIRQ(EXTI0_IRQn);

	     // Initialisation de la bibliothèque NEAI
	     if(neai_anomalydetection_init(use_pretrained) != NEAI_OK)
	     {
	    	 Error_Handler();
	     }

	     // Allume la LED bleue  (indique que l'apprentissage commence)
	     HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);
	     for (int i=0; i<2 ;i++)
	     {
	    	// detection des valeurs + affectation des valeurs vers input_signal
	    	acquisition_ok = DHT11_Acquire_ForNEAI(input_signal);
	    	// Envoie les données au moteur d'apprentissage NEAI
	    	state = neai_anomalydetection_learn(input_signal);
	    	microDelay(2500000);
	     }

	     // Éteint la LED bleue (fin de l'apprentissage)
	     HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);

	     // Si le nombre d'appels d'apprentissage est insuffisant, envoi un message via UART
	     if(state==NEAI_LEARNING_IN_PROGRESS)
	     {
	        char *msg1 = "Learning: Pas encore suffisant\r\n";
	        HAL_UART_Transmit(&huart2, (uint8_t*)msg1, strlen(msg1), 100);
	     }

	     //Si le nombre minimal recommandé est atteint, envoi un message via UART
	     else if(state==NEAI_LEARNING_DONE)
	     {
	        char *msg2 = "Learning: Phase minimale atteinte\r\n";
	        HAL_UART_Transmit(&huart2, (uint8_t*)msg2, strlen(msg2), 100);
	     }

	     // Réactive l'interruption externe (EXTI0) une fois l'apprentissage terminé
	     HAL_NVIC_EnableIRQ(EXTI0_IRQn);

}
/* USER CODE END 0 */

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
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start(&htim2);

  // --- NOUVEAU : Démarrer la réception UART par interruption ---
  memset(rxBuffer, 0, RX_BUFFER_SIZE);
  HAL_UART_Receive_IT(&huart2, &rxBuffer[0], 1); // Réception caractère par caractère

  if(neai_anomalydetection_init(use_pretrained) != NEAI_OK)
      {
     	 Error_Handler();
      }

  	   // Pause de 5 secondes pour stabilisation ou préparation
       HAL_Delay(5000);

       // Allume la LED bleue (indique que l'apprentissage commence)
       HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_SET);
       while (learn_count < 20)
       {
    	   // detection des valeurs + affectation des valeurs vers input_signal
    	   acquisition_ok = DHT11_Acquire_ForNEAI(input_signal);
    	   // Verifier l'aquisition + Envoie les données au moteur d'apprentissage NEAI
    	   if ( acquisition_ok )
    	   {

    		   state = neai_anomalydetection_learn(input_signal);
    		   learn_count++;
    		   char msg00[60];

    		   snprintf(msg00, sizeof(msg00), "Apprentissage %d/20 -> T=%dC H=%d%%\r\n",
    		            learn_count, (int)input_signal[0], (int)input_signal[1]);

    		   HAL_UART_Transmit(&huart2, (uint8_t *)msg00, strlen(msg00), HAL_MAX_DELAY);
    	   }
    	   else
    	   {

    		   char *msg01 = "Echec lecture DHT22, nouvelle tentative...\r\n";
    		   HAL_UART_Transmit(&huart2, (uint8_t*)msg01, strlen(msg01), 100);

    	   }

    	   // le DHT22 a besoin d'au moins 2 secondes entre deux lectures
    	       HAL_Delay(2500);

       }

       // Éteint la LED bleue (fin de l'apprentissage)
       HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET);

       // Si le nombre d'appels d'apprentissage est insuffisant, envoi un message via UART
            if(state == NEAI_LEARNING_IN_PROGRESS)
            {
           	 char *msg1 = "Learning: Pas encore suffisant\r\n";
           	 HAL_UART_Transmit(&huart2, (uint8_t*)msg1, strlen(msg1), 100);
            }

            //Si le nombre minimal recommandé est atteint, envoi un message via UART
            else if(state == NEAI_LEARNING_DONE)
            {
           	 char *msg2 = "Learning: Phase minimale atteinte\r\n";
           	 HAL_UART_Transmit(&huart2, (uint8_t*)msg2, strlen(msg2), 100);
            }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

      // ========== TRAITEMENT COMMANDE LED (INTERRUPTION) ==========
         if (commandeLedRecue)
         {
             commandeLedRecue = false; // Reset le flag

             // Commande spéciale "all" (allume/éteint toutes les LEDs)
                if (ledNumero == 0 && strncmp((char*)rxBuffer, "LED", 3) == 0)
                {
                    if (strstr((char*)rxBuffer, "all") != NULL)
                    {
                        // Vérifie si c'est allumage ou extinction
                        if (strstr((char*)rxBuffer, ":1") != NULL) {
                            // Allume toutes les LEDs
                            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);   // Verte
                            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);   // Orange
                            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);   // Rouge
                            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_SET);   // Bleue
                        } else {
                            // Éteint toutes les LEDs
                            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET); // Verte
                            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_RESET); // Orange
                            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET); // Rouge
                            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET); // Bleue
                        }
                    }
                }
                // Commandes individuelles LED 1-4
                else
                {
             switch (ledNumero)
             {
                 case 1:
                     HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_12); // LED Verte
                     break;
                 case 2:
                     HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_13); // LED Orange
                     break;
                 case 3:
                     HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_14); // LED Rouge
                     break;
                 case 4:
                     HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_15); // LED Bleue
                     break;
             }
                }

             // Debug : envoie confirmation
             char confirm[30];
             snprintf(confirm, sizeof(confirm), "LED%d toggled\r\n", ledNumero);
             HAL_UART_Transmit(&huart2, (uint8_t*)confirm, strlen(confirm), 100);
         }
         // ========== TRAITEMENT NEAI ==========
	  DHT11_Acquire_ForNEAI(input_signal);

	  // Appel à la fonction de détection NEAI, renvoie une similarité
	  	  if(neai_anomalydetection_detect(input_signal, &similarity)!=NEAI_OK)
	  	  {
	  	      Error_Handler();
	  	  }

	  	  // Si la similarité est supérieure ou égale à 90 (comportement normal)
	  	  if(similarity>=90)
	  	  {
	  		char msg3[80];
	  		    snprintf(msg3, sizeof(msg3), "NOMINAL T=%dC H=%d%%\r\n",
	  		             (int)input_signal[0], (int)input_signal[1]);
	  		  if(HAL_UART_Transmit(&huart2, (uint8_t*)msg3, strlen(msg3), 100)!=HAL_OK)
	  	      {
	  	        Error_Handler();
	  	      }
	  		  //HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET); // allumer led green
	  		  //HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_RESET); // eteindre led orange
	  	  }

	  	  // Si la similarité est inférieure à 90 (anomalie détectée)
	  	  else
	  	  {
	  		 char msg4[80];
	  		    snprintf(msg4, sizeof(msg4), "ANOMALY T=%dC H=%d%%\r\n",
	  		             (int)input_signal[0], (int)input_signal[1]);
	  		  if(HAL_UART_Transmit(&huart2, (uint8_t*)msg4, strlen(msg4), 100)!=HAL_OK)
	  	      {
	  	        Error_Handler();
	  	      }
	  		  //HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);
	  		  //HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);
	  	  }
	        HAL_Delay(2000);



    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 83;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB11 */
  GPIO_InitStruct.Pin = GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PD12 PD13 PD14 PD15 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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
  HAL_GPIO_WritePin(GPIOD,GPIO_PIN_14,GPIO_PIN_SET);
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
