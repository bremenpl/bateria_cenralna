/*
 * u_modBusGeneric.h
 *
 *  Created on: 14 lis 2016
 *      Author: Lukasz
 *
 *      index:
 *      - mbg: ModBus Generic
 *      - mbs: ModBus Slave
 *      - mbm: ModBus Master
 */

#ifndef U_MODBUSGENERIC_H_
#define U_MODBUSGENERIC_H_


/* Includes ------------------------------------------------------------------*/
#if defined(STM32F030x8) || defined(STM32F091xC)
	#include "stm32f0xx_hal.h"
#endif

#include "cmsis_os.h"
#include "stdbool.h"

/* Defines and macros --------------------------------------------------------*/
#define MBG_MAX_DATA_LEN				252

// addr + func + data + crc
#define MBG_MAX_FRAME_LEN				(1 + 1 + MBG_MAX_DATA_LEN + 2)
#define MBG_MAX_QTY_OF_REGS				((MBG_MAX_DATA_LEN / 2) - 1)

// lacking UART_Interrupt_definition
// Z = 11, X = 1, Y = 26
#define UART_IT_RTOF                    ((uint16_t)0x0B3A)
#define UART_CLEAR_RTOCF				USART_ICR_RTOCF

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
 * @brief	Machine state used for frames parsing during reception
 */
typedef enum
{
	e_mbsRxState_none				= 0,
	e_mbsRxState_addr,
	e_mbsRxState_funcCode,
	e_mbsRxState_data,	// to get the length
	e_mbsRxState_data2, // to get the bytes
	e_mbsRxState_crc,
} mbgRxState_t;

/*
 * @brief	Slave replies exception codes
 */
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

} mbgExCode_t;

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
} mbgFrame_t;

/*
 * @brief	Generic modbus substruct containing data for modbus master/ slave operation.
 */
typedef struct
{
	UART_HandleTypeDef* handle;					/*!< Pointer to a uart struct */
	osMutexId			txMut;					/*!< Transfer mutex for the uart */
	uint8_t 			buf[MBG_MAX_FRAME_LEN];	/*!< Operations buffer (tx and rx) */
	uint32_t			len;					/*!< Actual buffer size */

	struct										/*!< Container for rx queue objects */
	{
		osMessageQId 	msgQId_rX;				/*!< Receiver message queue object */
		osThreadId 		sysId_rX;				/*!< Receiver thread for \ref msgQId_rX */
		mbgFrame_t* 	frames;					/*!< Received queues buffer, needs to be
												allocated in the init function */
		uint32_t 		framesIndex;			/*!< Used for indexing \ref frames */
		uint32_t		framesBufLen;			/*!< Allocate \ref frames with this number */
	} rxQ;

	mbgRxState_t		rxState;				/*!< Current modbus receiver state */

} mbgUart_t;


/* Private variables ---------------------------------------------------------*/

/* Public variables ----------------------------------------------------------*/

// IMPORTANT: Modbus generic module should not have any public variables.

/* Fuction prototypes --------------------------------------------------------*/
HAL_StatusTypeDef mbg_UartInit(mbgUart_t* uart);
HAL_StatusTypeDef mbg_CheckCrc(const mbgFrame_t* const mf, uint16_t* crc);
HAL_StatusTypeDef mbg_SendFrame(mbgUart_t* uart, mbgFrame_t* mf);
void mbg_EnableRxTimeout(UART_HandleTypeDef* uHandle);
void mbg_DisableRxTimeout(UART_HandleTypeDef* uHandle);
void mbg_ClearRTOCF_Flag(UART_HandleTypeDef* uHandle);
HAL_StatusTypeDef mbg_EnableReceiver(UART_HandleTypeDef* uHandle,
		uint8_t* data, const uint16_t len, const uint32_t enableRxTimeout);

// overrides
void mbs_uartRxRoutine(UART_HandleTypeDef* uHandle);
void mbm_uartRxRoutine(UART_HandleTypeDef* uHandle);
void mbs_uartTxRoutine(UART_HandleTypeDef* uHandle);
void mbm_uartTxRoutine(UART_HandleTypeDef* uHandle);

void mbs_uartRxTimeoutRoutine(UART_HandleTypeDef* uHandle);
void mbm_uartRxTimeoutRoutine(UART_HandleTypeDef* uHandle);

/* Function declarations -----------------------------------------------------*/



#endif /* U_MODBUSGENERIC_H_ */
