/*
 * u_modBusMaster.h
 *
 *  Created on: 04.12.2016
 *      Author: Lukasz
 */

#ifndef U_MODBUSMASTER_H_
#define U_MODBUSMASTER_H_


/* Includes ------------------------------------------------------------------*/
#include "u_modBusGeneric.h"

/* Defines and macros --------------------------------------------------------*/

#define MBM_RCV_QUEUE_SIZE		2

/* Enums and structs ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Public variables ----------------------------------------------------------*/

/* Fuction prototypes --------------------------------------------------------*/
HAL_StatusTypeDef mbm_Init(UART_HandleTypeDef* uartHandle);
HAL_StatusTypeDef mbm_RequestReadHoldingRegs(uint8_t slaveAddr,
									  uint16_t startAddr,
									  uint16_t nrOfRegs);

void mbm_RespParseErrorSlaveAddr(uint8_t addr);
void mbm_RespParseErrorFuncCode(mbgExCode_t code);
void mbm_RespParseErrorData(const uint8_t* const data, uint16_t len);
void mbm_RespParseError();

void mbm_RespRxTimeoutError(mbgRxState_t state, const mbgFrame_t* const mf);
void mbs_CheckReadHoldingRegistersRequest(const mbgFrame_t* const mf);


/* Function declarations -----------------------------------------------------*/



#endif /* U_MODBUSMASTER_H_ */
