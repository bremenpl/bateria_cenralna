/*
 * u_modBusMaster.h
 *
 *  Created on: 02.03.2017
 *      Author: Lukasz
 */

#ifndef U_MODBUSMASTER_H_
#define U_MODBUSMASTER_H_


/* Includes ------------------------------------------------------------------*/

#include "u_modBusGeneric.h"

/* Defines and macros --------------------------------------------------------*/

#define MBS_MAX_NO_OF_MASTERS	2

/* Enums and structs ---------------------------------------------------------*/

typedef struct
{
	mbgUart_t			mbg;				/*!< Generic modbus functions */

	mbgFrame_t 			sendFrame;			/*!< Stores request data */
	uint32_t			retrMax;			/*!< How many times try to get response */
	uint32_t			retrMade;			/*!< How many tries were made already */

	struct
	{
		osMessageQId 	msgQId;				/*!< Response timeout queue */
		osThreadId 		sysId;				/*!< Response timeout execution thread ID */
		uint32_t 		timeout_ms;			/*!< Response timeout expressed in ms */
	} toutQ;

} mbmUart_t;

/* Private variables ---------------------------------------------------------*/

/* Public variables ----------------------------------------------------------*/

/* Fuction prototypes --------------------------------------------------------*/

HAL_StatusTypeDef mbm_Init(mbmUart_t* mbmu, size_t noOfModules);

HAL_StatusTypeDef mbm_RequestReadHoldingRegs(mbmUart_t* mbmu,
									  uint8_t slaveAddr,
									  uint16_t startAddr,
									  uint16_t nrOfRegs);
HAL_StatusTypeDef mbm_RequestWriteSingleCoil(mbmUart_t* mbmu,
									  uint8_t slaveAddr,
									  uint16_t outputAddr,
									  uint8_t state);

void mbm_RespParseErrorSlaveAddr(uint8_t addr);
void mbm_RespParseErrorFuncCode(mbgExCode_t code);
void mbm_RespParseErrorData(const uint8_t* const data, uint16_t len);
void mbm_RespParseError();
void mbm_RespRxTimeoutError(mbgRxState_t state, const mbgFrame_t* const mf);
void mbm_RespCrcMatchError(uint16_t rcvCrc, uint16_t calcCrc);

void mbm_CheckReadHoldingRegisters(const mbmUart_t* const mbm, const mbgFrame_t* const mf);

/* Function declarations -----------------------------------------------------*/


#endif /* U_MODBUSMASTER_H_ */
