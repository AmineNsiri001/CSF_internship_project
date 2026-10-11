/*
 * dht22.h
 *
 *  Created on: Apr 13, 2026
 *      Author: baker
 */

#ifndef INC_DHT22_H_
#define INC_DHT22_H_

#include "stm32f4xx_hal.h"

/*----USER CONFIG-----*/
#define DHT22_PORT GPIOB
#define DHT22_PIN  GPIO_PIN_11
#define TIMER htim1

extern TIM_HandleTypeDef htim1; // change your handler here accordingly

/*----FUNCTIONS----*/
void microDelay(uint16_t delay);
uint8_t DHT22_Start(void);
uint8_t DHT22_Read(void);
void DHT22_Read_All(float *tCels, float *tFahr, float *rh);



#endif /* INC_DHT22_H_ */
