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
#define MBS_MAX_NO_OF_SLAVES	4

#define MBS_MIN_ADDR			1
#define MBS_MAX_ADDR			247

#define MBS_EXCODE_ADDR_OR		0x80

/* Enums and structs ---------------------------------------------------------*/

typedef struct
{
	mbgUart_t			mbg;			/*!< Generic modbus functions */
	uint8_t				slaveAddr;		/*!< Modbus slave address */

} mbsUart_t;

/* Private variables ---------------------------------------------------------*/

/* Public variables ----------------------------------------------------------*/

/* Fuction prototypes --------------------------------------------------------*/
HAL_StatusTypeDef mbs_Init(mbsUart_t* mbsu, size_t noOfModules);

// override functions
mbgExCode_t mbs_CheckReadHoldingRegistersRequest(mbgFrame_t* mf);
mbgExCode_t mbs_CheckReadCoils(mbgFrame_t* mf);
mbgExCode_t mbs_CheckWriteSingleCoil(mbgFrame_t* mf);

/* Function declarations -----------------------------------------------------*/


#endif /* U_MODBUSSLAVE_H_ */
