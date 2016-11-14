/*
 * u_modBusGeneric.h
 *
 *  Created on: 14 lis 2016
 *      Author: Lukasz
 */

#ifndef U_MODBUSGENERIC_H_
#define U_MODBUSGENERIC_H_


/* Includes ------------------------------------------------------------------*/
#ifdef STM32F030x8
#include "stm32f0xx_hal.h"
#endif

#include "cmsis_os.h"

/* Defines and macros --------------------------------------------------------*/
#define MBG_MAX_DATA_LEN				252
#define MBG_MAX_QTY_OF_REGS				((MBG_MAX_DATA_LEN / 2) - 1)

/* Enums and structs ---------------------------------------------------------*/

/*
 * @brief	Public Function Code Definitions
 */
typedef enum
{
	e_mbFuncCode_ReadDiscreteInputs			= 02,
	e_mbFuncCode_ReadCoils 					= 01,
	e_mbFuncCode_WriteSingleCoil			= 05,
	e_mbFuncCode_WriteMultipleCoils			= 15,
	e_mbFuncCode_ReadInputRegister 			= 04,
	e_mbFuncCode_ReadHoldingRegisters		= 03,
	e_mbFuncCode_WriteSingleRegister		= 06,
	e_mbFuncCode_WriteMultipleRegisters		= 16,
	e_mbFuncCode_ReadWriteMultipleRegisters	= 23,
	e_mbFuncCode_MaskWriteRegister			= 22,

} modbusFuncCode_t;

/*
 * @brief	Structure representing single ModBus frame. The struct is optimised
 * 			for 32 bit padding (size should be 258 bytes).
 */
typedef struct
{
	uint8_t			addr;					/*!< Slave modbus address */
	uint8_t			code;					/*!< Frame function code */
	uint16_t		dataLen;				/*!< Data field length of the frame */
	uint8_t			data[MBG_MAX_DATA_LEN];	/*!< Raw data */
	uint16_t		crc;					/*!< CRC code */
} modbusFrame_t;

/* Private variables ---------------------------------------------------------*/

/* Public variables ----------------------------------------------------------*/

/* Fuction prototypes --------------------------------------------------------*/
uint16_t mbg_CalculateCrc(uint8_t* data, size_t len);

/* Function declarations -----------------------------------------------------*/



#endif /* U_MODBUSGENERIC_H_ */
