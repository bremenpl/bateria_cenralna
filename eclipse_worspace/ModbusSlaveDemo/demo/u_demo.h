/*
 * u_demo.h
 *
 *  Created on: 29 lis 2016
 *      Author: Lukasz
 */

#ifndef U_DEMO_H_
#define U_DEMO_H_


/* Includes ------------------------------------------------------------------*/
#ifdef STM32F030x8
	#include "stm32f0xx_hal.h"
#endif

#include "cmsis_os.h"

/* Defines and macros --------------------------------------------------------*/
#define DEMO_EXTI_GPIO			GPIOC
#define DEMO_EXTI_PIN			GPIO_PIN_13
#define DEMO_EXPI_ON_STATE		0

#define DEMO_LED_GPIO			GPIOA
#define DEMO_LED_PIN			GPIO_PIN_5

#define DEMO_REGISTER_ADDR		0x00

/* Enums and structs ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Public variables ----------------------------------------------------------*/

/* Fuction prototypes --------------------------------------------------------*/
HAL_StatusTypeDef demo_Init();

/* Function declarations -----------------------------------------------------*/


#endif /* U_DEMO_H_ */
