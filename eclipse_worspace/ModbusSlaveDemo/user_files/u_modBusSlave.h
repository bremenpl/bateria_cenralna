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

typedef enum
{
	e_mbsRxState_addr				= 0,
	e_mbsRxState_funcCode			= 1,
	e_mbsRxState_data 				= 2,
	e_mbsRxState_crc				= 3,
} mbsRxState_t;

typedef enum
{
	e_mbsExCode_noError				= 0, // used only for validation, not to be sent
	e_mbsExCode_illegalFunction		= 0x01,
	e_mbsExCode_illegalDataAddr		= 0x02,
	e_mbsExCode_illegalDataValue	= 0x03,
	e_mbsExCode_slaveDeviceFailure	= 0x04,
	e_mbsExCode_acknowledge			= 0x05,
	e_mbsExCode_slaveDeviceBusy		= 0x06,
	e_mbsExCode_memoryParityError	= 0x08,

} mbsExCode_t;

/* Private variables ---------------------------------------------------------*/

/* Public variables ----------------------------------------------------------*/

/* Fuction prototypes --------------------------------------------------------*/
HAL_StatusTypeDef mbs_Init(UART_HandleTypeDef* uartHandle, uint8_t address);
HAL_StatusTypeDef mbs_SendResponse(modbusFrame_t* mf);

// override functions
mbsExCode_t mbs_CheckReadHoldingRegistersRequest(modbusFrame_t* mf);

/* Function declarations -----------------------------------------------------*/


#endif /* U_MODBUSSLAVE_H_ */
