/*
 * u_demo.c
 *
 *  Created on: 29 lis 2016
 *      Author: Lukasz
 */


/* Includes ------------------------------------------------------------------*/
#include "u_demo.h"
#include "u_modBusSlave.h"

/* Defines and macros --------------------------------------------------------*/

/* Enums and structs ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
uint16_t demo_registerValue;

/* Public variables ----------------------------------------------------------*/

/* Fuction prototypes --------------------------------------------------------*/

/* Function declarations -----------------------------------------------------*/

/*
 * @brief	Demonstration functionalities initialization function
 * @return	HAL_OK if everything set correctly.
 */
HAL_StatusTypeDef demo_Init()
{
	return 0;
}

/*
 * @brief	extenal interrupt override function
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (DEMO_EXTI_PIN == GPIO_Pin)
	{
		// No debounce mechanisms needed for this function,
		// theres a capacitor on nucleo board.

		if (!HAL_GPIO_ReadPin(DEMO_EXTI_GPIO, DEMO_EXTI_PIN))
		{
			demo_registerValue ^= 1;
			HAL_GPIO_WritePin(DEMO_LED_GPIO, DEMO_LED_PIN, demo_registerValue);
		}
	}
}

/*
 * @brief	Demo override function for reading holding registers
 */
mbsExCode_t mbs_CheckReadHoldingRegistersRequest(modbusFrame_t* mf)
{
	if (!mf)
		return e_mbsExCode_illegalDataAddr;

	// start address
	uint16_t startAddr = mf->data[1] | ((uint16_t)mf->data[0] << 8);

	// quantity of registers
	uint16_t nrOfRegs = mf->data[3] | ((uint16_t)mf->data[2] << 8);

	// prevent reading wrong address
	if (DEMO_REGISTER_ADDR !=  startAddr)
		return e_mbsExCode_illegalDataAddr;

	// demo has only 1 register, prevent reading more
	if (1 != nrOfRegs)
		return e_mbsExCode_illegalDataAddr;

	// form the frame: slave address and function code remain unchanged
	// total len
	mf->dataLen = 3; // byte count + register value

	// byte count
	mf->data[0] = 2;

	// register value:
	mf->data[1] = (uint8_t)(demo_registerValue >> 8);
	mf->data[2] = (uint8_t)demo_registerValue;

	// return no error
	return e_mbsExCode_noError;
}


