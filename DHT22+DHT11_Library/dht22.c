/*
 * dht22.c
 *
 *  Created on: Apr 13, 2026
 *      Author: baker
 */


#include "dht22.h"

/**
  *microsecond delay
  */
void microDelay(uint16_t delay)
{
  __HAL_TIM_SET_COUNTER(&TIMER, 0);
  while (__HAL_TIM_GET_COUNTER(&TIMER) < delay);
}

/**
  *START DHT22
  */
uint8_t DHT22_Start (void)
{
	uint32_t pMillis, cMillis;
  uint8_t Response = 0;
  GPIO_InitTypeDef GPIO_InitStructPrivate = {0};
  GPIO_InitStructPrivate.Pin = DHT22_PIN;
  GPIO_InitStructPrivate.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructPrivate.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStructPrivate.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(DHT22_PORT, &GPIO_InitStructPrivate); // set the pin as output
  HAL_GPIO_WritePin (DHT22_PORT, DHT22_PIN, 0);   // pull the pin low
  microDelay (1300);   // wait for 1300us
  HAL_GPIO_WritePin (DHT22_PORT, DHT22_PIN, 1);   // pull the pin high
  microDelay (30);   // wait for 30us
  GPIO_InitStructPrivate.Mode = GPIO_MODE_INPUT;
  GPIO_InitStructPrivate.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(DHT22_PORT, &GPIO_InitStructPrivate); // set the pin as input
  microDelay (40);
  if (!(HAL_GPIO_ReadPin (DHT22_PORT, DHT22_PIN)))
  {
    microDelay (80);
    if ((HAL_GPIO_ReadPin (DHT22_PORT, DHT22_PIN))) Response = 1;
  }
  pMillis = HAL_GetTick();
  cMillis = HAL_GetTick();
  while ((HAL_GPIO_ReadPin (DHT22_PORT, DHT22_PIN)) && pMillis + 2 > cMillis)
  {
    cMillis = HAL_GetTick();
  }
  return Response;
}

/**
  *READ BYTE
  */
uint8_t DHT22_Read(void)
{
  uint8_t i, b = 0;

  for (i = 0; i < 8; i++)
  {
    while (!HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN)); // wait HIGH

    microDelay(40);

    if (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN))
      b |= (1 << (7 - i));

    while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN)); // wait LOW
  }

  return b;
}

/**
  *READ ALL
  */
void DHT22_Read_All(float *tCels, float *tFahr, float *rh)
{
	uint8_t RH1, RH2, TC1, TC2, SUM, CHECK;
   if(DHT22_Start())
   {
	 RH1 = DHT22_Read(); // First 8bits of humidity
	 RH2 = DHT22_Read(); // Second 8bits of Relative humidity
	 TC1 = DHT22_Read(); // First 8bits of Celsius
     TC2 = DHT22_Read(); // Second 8bits of Celsius
	 SUM = DHT22_Read(); // Check sum
	 CHECK = RH1 + RH2 + TC1 + TC2;
	 if (CHECK == SUM)
	 {
		if (TC1>127) // If TC1=10000000, negative temperature
		{
		    *tCels = (float)TC2/10*(-1);
		}
		else
		{
		    *tCels = (float)((TC1<<8)|TC2)/10;
		}
		*tFahr = *tCels * 9/5 + 32;
		*rh = (float) ((RH1<<8)|RH2)/10;
	  }
	}
}

