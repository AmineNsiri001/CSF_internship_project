/*
 * dht11_timeout.c
 *
 *  Driver DHT11 avec protection par timeout.
 *  Adapte du fichier dht22.c original.
 *
 *  Difference cle avec le DHT22 : le DHT11 n'a pas de decimales.
 *  Les octets RH2 et TC2 (partie decimale) valent toujours 0 sur DHT11,
 *  donc PAS de division par 10 dans le calcul final.
 */

#include "dht11_timeout.h"

/**
  * microsecond delay
  */
void microDelay(uint16_t delay)
{
  __HAL_TIM_SET_COUNTER(&TIMER, 0);
  while (__HAL_TIM_GET_COUNTER(&TIMER) < delay);
}

/**
  * START DHT11
  */
uint8_t DHT11_Start(void)
{
  uint32_t pMillis, cMillis;
  uint8_t Response = 0;
  GPIO_InitTypeDef GPIO_InitStructPrivate = {0};
  GPIO_InitStructPrivate.Pin = DHT11_PIN;
  GPIO_InitStructPrivate.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructPrivate.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStructPrivate.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStructPrivate); // set the pin as output
  HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, 0);   // pull the pin low
  microDelay(18000);   // DHT11 : signal de start minimum 18ms (plus long que DHT22)
  HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, 1);   // pull the pin high
  microDelay(30);      // wait for 30us
  GPIO_InitStructPrivate.Mode = GPIO_MODE_INPUT;
  GPIO_InitStructPrivate.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStructPrivate); // set the pin as input
  microDelay(40);
  if (!(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)))
  {
    microDelay(80);
    if ((HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN))) Response = 1;
  }
  pMillis = HAL_GetTick();
  cMillis = HAL_GetTick();
  while ((HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)) && pMillis + 2 > cMillis)
  {
    cMillis = HAL_GetTick();
  }
  return Response;
}

/**
  * READ BYTE (avec timeout)
  * Retourne 1 si succes, 0 si timeout.
  */
uint8_t DHT11_Read(uint8_t *byte_out)
{
  uint8_t i, b = 0;

  for (i = 0; i < 8; i++)
  {
    // attente du front montant (HIGH), avec limite de temps
    __HAL_TIM_SET_COUNTER(&TIMER, 0);
    while (!HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN))
    {
      if (__HAL_TIM_GET_COUNTER(&TIMER) > DHT11_TIMEOUT_US) return 0; // timeout : capteur muet
    }

    microDelay(40);

    if (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN))
      b |= (1 << (7 - i));

    // attente du front descendant (LOW), avec limite de temps
    __HAL_TIM_SET_COUNTER(&TIMER, 0);
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN))
    {
      if (__HAL_TIM_GET_COUNTER(&TIMER) > DHT11_TIMEOUT_US) return 0; // timeout : ligne bloquee haute
    }
  }

  *byte_out = b;
  return 1;
}

/**
  * READ ALL (avec propagation des erreurs)
  * Retourne 1 si lecture + checksum valides, 0 sinon.
  *
  * IMPORTANT : formule adaptee au DHT11 (pas de division par 10),
  * contrairement au DHT22 qui a des decimales.
  */
uint8_t DHT11_Read_All(float *tCels, float *tFahr, float *rh)
{
  uint8_t RH1, RH2, TC1, TC2, SUM, CHECK;

  if (!DHT11_Start())
    return 0; // le capteur n'a pas repondu au signal de depart

  if (!DHT11_Read(&RH1)) return 0; // partie entiere humidite
  if (!DHT11_Read(&RH2)) return 0; // partie decimale humidite (toujours 0 sur DHT11)
  if (!DHT11_Read(&TC1)) return 0; // partie entiere temperature
  if (!DHT11_Read(&TC2)) return 0; // partie decimale temperature (toujours 0 sur DHT11)
  if (!DHT11_Read(&SUM)) return 0;

  CHECK = RH1 + RH2 + TC1 + TC2;
  if (CHECK != SUM)
    return 0; // donnees corrompues (checksum invalide)

  // DHT11 : pas de decimales -> pas de division par 10
  *tCels = (float)TC1;
  *tFahr = *tCels * 9 / 5 + 32;
  *rh = (float)RH1;

  return 1;
}

/**
  * ACQUISITION POUR NANOEDGE AI
  * Remplit neai_buffer[0] = temperature (C), neai_buffer[1] = humidite (%).
  * Retourne 1 si succes, 0 si echec (buffer non modifie).
  */
uint8_t DHT11_Acquire_ForNEAI(float *neai_buffer)
{
  float tCels, tFahr, rh;

  if (!DHT11_Read_All(&tCels, &tFahr, &rh))
    return 0;

  neai_buffer[0] = tCels;
  neai_buffer[1] = rh;
  return 1;
}
