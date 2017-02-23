/*
 * u_address.c
 *
 *  Created on: 18.02.2017
 *      Author: Lukasz
 */

/* Includes ------------------------------------------------------------------*/

#include "u_address.h"

/* Defines and macros --------------------------------------------------------*/

/* Enums and structs ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Public variables ----------------------------------------------------------*/

/* Fuction prototypes --------------------------------------------------------*/

/* Function declarations -----------------------------------------------------*/

/*
 * @brief	Obtains the address from provided address struct.
 * @param	addr: pointer under which address is saved.
 * @param	as: address struct containing pins description.
 * @param	format: The encoder format, currently 10 and 16 format supported.
 *
 * @return	HAL_OK if address sucesfully saved under \red addr.
 */
HAL_StatusTypeDef address_Init(uint8_t* addr, const addr_t* const as, const uint32_t format)
{
	assert_param(addr);
	assert_param(as);
	uint8_t tempAddrA = 0, tempAddrB = 0;

	// 1st 4 bits (ENCA)
	for (uint32_t i = 0; i < ENC_BITS; i++)
	{
		// check gpio
		if (!as->enca[i].gpio)
			return HAL_ERROR;

		if (!HAL_GPIO_ReadPin(as->enca[i].gpio, as->enca[i].nr)) // pin is shorted
			tempAddrA |= 1 << i;
	}

	// 2nd 4 bits (ENCB)
	for (uint32_t i = 0; i < ENC_BITS; i++)
	{
		// check gpio
		if (!as->encb[i].gpio)
			return HAL_ERROR;

		if (!HAL_GPIO_ReadPin(as->encb[i].gpio, as->encb[i].nr)) // pin is shorted
			tempAddrB |= 1 << i;
	}

	// check if not zero (broadcast)
	if (!(tempAddrA | tempAddrB))
		return HAL_ERROR;

	// choose format
	switch (format)
	{
		case 16:
		{
			tempAddrA |= tempAddrB << ENC_BITS;
			break;
		}

		case 10:
		{
			tempAddrA = tempAddrB * 10 + tempAddrA;
			break;
		}

		default: return HAL_ERROR;
	}

	// assign
	*addr = tempAddrA;
	return HAL_OK;
}





