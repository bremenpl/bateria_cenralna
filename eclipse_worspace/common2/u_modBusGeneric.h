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
#if defined(STM32F030x8) || defined(STM32F091xC) || defined(STM32F051x8) || defined(STM32F098xx)
	#include "stm32f0xx_hal.h"
#endif

#include "cmsis_os.h"
#include "stdbool.h"
#include "u_plc.h"

/* Defines and macros --------------------------------------------------------*/
#define MBG_MAX_DATA_LEN				252
#define MBG_RECV_QUEUE_LEN				16

// addr + func + data + crc
#define MBG_MAX_FRAME_LEN				(1 + 1 + MBG_MAX_DATA_LEN + 2)
#define MBG_MAX_QTY_OF_REGS				((MBG_MAX_DATA_LEN / 2) - 1)

#ifndef HAL_UART_MODULE_ENABLED
	#define HAL_UART_STATE_READY 0
	#define HAL_UART_STATE_BUSY_RX 0
	typedef uint32_t UART_HandleTypeDef;
	typedef uint32_t HAL_UART_StateTypeDef;
#endif

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
	e_mbgRxState_none				= 0,
	e_mbgRxState_addr,
	e_mbgRxState_funcCode,
	e_mbgRxState_data,	// to get the length
	e_mbgRxState_data2, // to get the bytes
	e_mbgRxState_crcHi, //
	e_mbgRxState_crcLo,
	e_mbgRxState_waitForTout,
	e_mbgRxState_waitForToutRdyFrame,

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

	e_mbsExCode_delegate			= 0xFF, // non standard modbus code

} mbgExCode_t;

typedef enum
{
	e_mbgMesgType_Request		= 0,
	e_mbgMesgType_Response		= 1,

} mbgMesgType_t;

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
	mbgMesgType_t	msgType;				/*!< Is it a request or a response? */

} mbgFrame_t;

typedef enum
{
	e_mbgModuleType_Uart	= 0,
	e_mbgModuleType_Plc		= 1,
} mbgModuleType_t;

/*
 * @brief	Generic modbus substruct containing data for modbus master/ slave operation.
 */
typedef struct
{
	mbgModuleType_t		modType;				/*!< Regular uart or plc module */
	UART_HandleTypeDef* handle;					/*!< Pointer to a uart struct */
	plcm_t				plcm;					/*!< PLC module */
	uint32_t			plcId;					/*!< PLC module ID after init */
	uint8_t 			txbuf[MBG_MAX_FRAME_LEN];	/*!< Operations buffer (tx) */
	uint32_t			len;						/*!< Actual buffer size */

	struct										/*!< Container for rx queue objects */
	{
		osMessageQId 		msgQId;				/*!< Receiver message queue object */
		osThreadId 			sysId;				/*!< Receiver thread for \ref msgQId_rX */
		TIM_HandleTypeDef*	toutTim;			/*!< Rx timeout timer */
		float				T35;				/*!< nr of signs for timeout */
		uint8_t				rxByte;				/*!< byte saved at rx interrupt */
	} rxQ;

	mbgRxState_t		rxState;				/*!< Current modbus receiver state */
	mbgFrame_t			rxFrame;				/*!< receiver frame */
	uint32_t			mbTimeout_ms;			/*!< T3.5 timeout expressed in ms */

} mbgUart_t;


/* Private variables ---------------------------------------------------------*/

/* Public variables ----------------------------------------------------------*/

// IMPORTANT: Modbus generic module should not have any public variables.

/* Fuction prototypes --------------------------------------------------------*/
HAL_StatusTypeDef mbg_UartInit(mbgUart_t* uart);
HAL_StatusTypeDef mbg_CheckCrc(const mbgFrame_t* const mf, uint16_t* crc);
HAL_StatusTypeDef mbg_SendFrame(mbgUart_t* uart, mbgFrame_t* mf);
//void mbg_getUniqId(uint16_t* id);
HAL_StatusTypeDef mbg_ByteReceived(mbgUart_t* uart);
HAL_StatusTypeDef mbg_RxTimeout(mbgUart_t* uart);
int mbg_inHandlerMode();
HAL_StatusTypeDef mbg_DisableReceiver(mbgUart_t* uart);

// overrides for uart
void mbs_uartRxRoutine(UART_HandleTypeDef* uHandle);
void mbm_uartRxRoutine(UART_HandleTypeDef* uHandle);
void mbs_uartTxRoutine(UART_HandleTypeDef* uHandle);
void mbm_uartTxRoutine(UART_HandleTypeDef* uHandle);

// overrides for plc
void mbs_plcRxRoutine(plcm_t* plcHandle);
void mbm_plcRxRoutine(plcm_t* plcHandle);
void mbs_plcTxRoutine(plcm_t* plcHandle);
void mbm_plcTxRoutine(plcm_t* plcHandle);

// generic
void mbs_uartRxTimeoutRoutine(TIM_HandleTypeDef* tim);
void mbm_uartRxTimeoutRoutine(TIM_HandleTypeDef* tim);

// overrides for generic purpose
void mbg_uartPrintFrame(const mbgFrame_t* const mf);

/* Function declarations -----------------------------------------------------*/



#endif /* U_MODBUSGENERIC_H_ */
