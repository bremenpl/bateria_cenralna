/*
 * u_address.h
 *
 *  Created on: 18.02.2017
 *      Author: Lukasz
 */

#ifndef U_ADDRESS_H_
#define U_ADDRESS_H_

/* Includes ------------------------------------------------------------------*/

#if defined(STM32F030x8) || defined(STM32F091xC) || defined(STM32F051x8) || defined(STM32F098xx)
	#include "stm32f0xx_hal.h"
#endif

/* Defines and macros --------------------------------------------------------*/

#define ENC_BITS	4

/* Enums and structs ---------------------------------------------------------*/

typedef struct
{
	uint16_t		nr;			/*!< Pin number in shift format */
	GPIO_TypeDef*	gpio;		/*!< GPIO struct pointer */

} gpio_t;

typedef struct
{
	gpio_t	enca[ENC_BITS];		/*!< Encoder A lines */
	gpio_t	encb[ENC_BITS];		/*!< Encoder B lines */

} addr_t;

/* Private variables ---------------------------------------------------------*/

/* Public variables ----------------------------------------------------------*/

/* Fuction prototypes --------------------------------------------------------*/

/* Function declarations -----------------------------------------------------*/

HAL_StatusTypeDef address_Init(uint8_t* addr, const addr_t* const as, const uint32_t format);


#endif /* U_ADDRESS_H_ */
