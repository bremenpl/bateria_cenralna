/*
 * u_modBusSlave.h
 *
 *  Created on: 14 lis 2016
 *      Author: Lukasz
 */

#ifndef U_MODBUSSLAVE_H_
#define U_MODBUSSLAVE_H_



/* Includes ------------------------------------------------------------------*/
#include "u_modBusGeneric.h"

/* Defines and macros --------------------------------------------------------*/
#define MBS_RCV_QUEUE_SIZE	2

/* Enums and structs ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Public variables ----------------------------------------------------------*/

/* Fuction prototypes --------------------------------------------------------*/
HAL_StatusTypeDef mbs_Init(UART_HandleTypeDef* uartHandle);

/* Function declarations -----------------------------------------------------*/


#endif /* U_MODBUSSLAVE_H_ */
