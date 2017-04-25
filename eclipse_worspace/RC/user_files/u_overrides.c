/*
 * u_overrides.c
 *
 *  Created on: 10.04.2017
 *      Author: Lukasz
 */


/* Includes ------------------------------------------------------------------*/

#include "stm32f0xx_hal.h"
#include "cmsis_os.h"

#include "u_modBusGeneric.h"
#include "u_plc.h"

/* Defines and macros --------------------------------------------------------*/

/* Enums and structs ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Public variables ----------------------------------------------------------*/

/* Fuction prototypes --------------------------------------------------------*/

/* Function declarations -----------------------------------------------------*/

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	// check if SLAVE handled
	mbs_uartRxTimeoutRoutine(htim);

	// check if MASTER handled
	mbm_uartRxTimeoutRoutine(htim);

	// check if plcm timeout
	plcm_timHandle(htim);
}

/*
 * @brief	External interrupts handler, used for #ACFAIL notification (ZC)
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	plcm_extiHandle(GPIO_Pin);
}







