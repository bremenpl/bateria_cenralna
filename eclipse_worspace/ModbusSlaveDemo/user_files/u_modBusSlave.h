/*
 * u_modBusSlave.h
 *
 *  Created on: 14 lis 2016
 *      Author: Lukasz
 *
 */

#ifndef U_MODBUSSLAVE_H_
#define U_MODBUSSLAVE_H_



/* Includes ------------------------------------------------------------------*/
#include "u_modBusGeneric.h"

/* Defines and macros --------------------------------------------------------*/
#define MBS_RCV_QUEUE_SIZE	2

#define MBS_MIN_ADDR		1
#define MBS_MAX_ADDR		247

/* Enums and structs ---------------------------------------------------------*/

typedef enum
{
	e_mbsRxState_addr		= 0,
	e_mbsRxState_funcCode	= 1,
	e_mbsRxState_data 		= 2,
	e_mbsRxState_crc		= 3,
} mbsRxState_t;

/* Private variables ---------------------------------------------------------*/

/* Public variables ----------------------------------------------------------*/

/* Fuction prototypes --------------------------------------------------------*/
HAL_StatusTypeDef mbs_Init(UART_HandleTypeDef* uartHandle, uint8_t address);

/* Function declarations -----------------------------------------------------*/


#endif /* U_MODBUSSLAVE_H_ */
