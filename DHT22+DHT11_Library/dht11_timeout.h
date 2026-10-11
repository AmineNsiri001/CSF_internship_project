/*
 * dht11_timeout.h
 *
 *  Driver DHT11 avec protection par timeout.
 *  Adapte du fichier dht22.h original (protocole identique au DHT22,
 *  mais formule de conversion differente : pas de decimales sur DHT11).
 */

#ifndef INC_DHT11_TIMEOUT_H_
#define INC_DHT11_TIMEOUT_H_

#include "stm32f4xx_hal.h"

/*----USER CONFIG-----*/
#define DHT11_PORT GPIOB
#define DHT11_PIN  GPIO_PIN_11
#define TIMER      htim2   // adapte selon ton timer configure en microsecondes

/* Timeout de securite en microsecondes pour chaque front attendu. */
#define DHT11_TIMEOUT_US 150

extern TIM_HandleTypeDef htim2; // adapte selon ton handler reel

/*----FUNCTIONS----*/
void microDelay(uint16_t delay);
uint8_t DHT11_Start(void);

/* DHT11_Read : lit un octet.
 * Retourne 1 si succes, 0 si timeout. */
uint8_t DHT11_Read(uint8_t *byte_out);

/* DHT11_Read_All : lit temperature + humidite (entiers, sans decimale).
 * Retourne 1 si lecture ET checksum valides, 0 sinon. */
uint8_t DHT11_Read_All(float *tCels, float *tFahr, float *rh);

/* DHT11_Acquire_ForNEAI : remplit un buffer pret pour NanoEdge AI.
 * neai_buffer[0] = temperature (C), neai_buffer[1] = humidite (%).
 * Retourne 1 si succes, 0 si echec. */
uint8_t DHT11_Acquire_ForNEAI(float *neai_buffer);

#endif /* INC_DHT11_TIMEOUT_H_ */
