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
#define MBS_RCV_QUEUE_SIZE		2

#define MBS_MIN_ADDR			1
#define MBS_MAX_ADDR			247

#define MBS_EXCODE_ADDR_OR		0x80

/* Enums and structs ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Public variables ----------------------------------------------------------*/

/* Fuction prototypes --------------------------------------------------------*/
HAL_StatusTypeDef mbs_Init(UART_HandleTypeDef* uartHandle, uint8_t address);

// override functions
mbgExCode_t mbs_CheckReadHoldingRegistersRequest(mbgFrame_t* mf);
mbgExCode_t mbs_CheckReadCoils(mbgFrame_t* mf);
mbgExCode_t mbs_CheckWriteSingleCoil(mbgFrame_t* mf);

/* Function declarations -----------------------------------------------------*/


#endif /* U_MODBUSSLAVE_H_ */
