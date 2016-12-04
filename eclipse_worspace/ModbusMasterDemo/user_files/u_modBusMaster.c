/*
 * u_modBusMaster.c
 *
 *  Created on: 04.12.2016
 *      Author: Lukasz
 */



/* Includes ------------------------------------------------------------------*/
#include "u_modBusMaster.h"

/* Defines and macros --------------------------------------------------------*/

/* Enums and structs ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

static osMessageQId mbm_msgQId_rX;
static osThreadId mbm_sysId_rX;
static mbgFrame_t mbm_rxFrames[MBM_RCV_QUEUE_SIZE];
static uint32_t mbm_rxFramesIndex;

static mbgUart_t mbm_uart;
static mbgRxState_t mbm_rxState;
static mbgFrame_t mbm_sentFrame;

/* Public variables ----------------------------------------------------------*/

/* Fuction prototypes --------------------------------------------------------*/
void mbm_task_rxDequeue(void const* argument);
HAL_StatusTypeDef mbm_ExamineFuncCode(const uint8_t fCode, uint8_t** data_p, uint32_t* len);
HAL_StatusTypeDef mbm_ExamineData(const mbgFrame_t* const mf,
								  uint8_t** data_p,
								  uint32_t* len);

/* Function declarations -----------------------------------------------------*/

/*
 * @brief	Initializes the MODBUS Master controller.
 * @param	uartHandle: pointer to a uart HAL handle struct.
 * @return	HAL_OK if initialization was succesfull.
 */
HAL_StatusTypeDef mbm_Init(UART_HandleTypeDef* uartHandle)
{
	if (!uartHandle)
		return HAL_ERROR;

	HAL_StatusTypeDef retVal = HAL_OK;

	// assign handle, initialize uart
	mbm_uart.handle = uartHandle;
	retVal += mbg_UartInit(&mbm_uart);

	// create slave response reception queue.
	osMessageQDef_t msgDef_temp;
	msgDef_temp.item_sz = sizeof(mbgFrame_t*);
	msgDef_temp.queue_sz = MBM_RCV_QUEUE_SIZE;

	// create the queue
	mbm_msgQId_rX = osMessageCreate(&msgDef_temp, NULL);
	if (!mbm_msgQId_rX)
		retVal++;

	// create thread for dequeuing
	osThreadDef(mbmRxTask, mbm_task_rxDequeue, osPriorityAboveNormal, 0, 256);
	mbm_sysId_rX = osThreadCreate(osThread(mbmRxTask), NULL);
	if (!mbm_sysId_rX)
		retVal++;

	// defaulr rx state
	mbm_rxState = e_mbsRxState_none;

	return retVal;
}

/*
 * @brief	Read holding registers command request command.
 * @param	slaveAddr: Slave to which the request is addressed to.
 * @param	startAddr: Address of the register the reading starts from.
 * @param	nrOfRegs: Number of 16 bit registers to read, starting from \ref startAddr.
 * @param	HAL_OK if request sent succesfully.
 */
HAL_StatusTypeDef mbm_RequestReadHoldingRegs(uint8_t slaveAddr,
									  uint16_t startAddr,
									  uint16_t nrOfRegs)
{
	if (!nrOfRegs)
		return HAL_OK; // nothing to read

	// set slave addr
	mbm_sentFrame.addr = slaveAddr;

	// set function code
	mbm_sentFrame.code = e_mbFuncCode_ReadHoldingRegisters;

	// starting address HI and LO
	mbm_sentFrame.dataLen = 0;
	mbm_sentFrame.data[mbm_sentFrame.dataLen++] = (uint8_t)(startAddr >> 8);
	mbm_sentFrame.data[mbm_sentFrame.dataLen++] = (uint8_t)startAddr;

	// quantity of registers HI and :P
	mbm_sentFrame.data[mbm_sentFrame.dataLen++] = (uint8_t)(nrOfRegs >> 8);
	mbm_sentFrame.data[mbm_sentFrame.dataLen++] = (uint8_t)nrOfRegs;

	// send
	if (mbg_SendFrame(&mbm_uart, &mbm_sentFrame))
	{
		mbm_sentFrame.addr = 0;
		return HAL_ERROR;
	}

	return HAL_OK;
}

/*
 * @brief	Call this method in function code examination routine.
 * @param	fCode: Input parameter, received function code to examine.
 * @param	data_p: data pointer under which rx data will be saved,
 * @param	len: length of received data.
 * @return 	HAL_OK if function code recognized
 */
HAL_StatusTypeDef mbm_ExamineFuncCode(const uint8_t fCode, uint8_t** data_p, uint32_t* len)
{
	if (!fCode || !data_p || !len)
		return HAL_ERROR;

	HAL_StatusTypeDef retVal = HAL_OK;

	// decide what to do next depending on the function code
	switch (fCode)
	{
		case e_mbFuncCode_ReadHoldingRegisters:
		{
			// Byte count (1), then number of registers read from sent frame
			*data_p = mbm_rxFrames[mbm_rxFramesIndex].data;
			uint16_t noOfRegs = (mbm_sentFrame.data[2] << 8) | mbm_sentFrame.data[3];
			*len = 1 + noOfRegs * 2; // 1 for byte count and *2 because regs are 16 bit

			// save length for generic purposes, like crc calculation
			mbm_rxFrames[mbm_rxFramesIndex].dataLen = *len;
			break;
		}

		/*case e_mbFuncCode_WriteMultipleRegisters:
		{
			break;
		}*/

		default:
		{
			// same principle as for 1st byte
			*data_p = &mbm_rxFrames[mbm_rxFramesIndex].code;
			*len = MBG_MAX_DATA_LEN + 3;
			retVal =  HAL_ERROR; // unknown function code
		}
	}

	return retVal;
}

/*
 * @brief	Call this method in data examination routine.
 * @param	mf: pointer to the so far gathered frame.
 * @param	data_p: data pointer under which rx data will be saved,
 * @param	len: length of received data.
 * @return 	HAL_OK if function code recognized
 */
HAL_StatusTypeDef mbm_ExamineData(const mbgFrame_t* const mf,
								  uint8_t** data_p,
								  uint32_t* len)
{
	if (!mf || !data_p || !len)
		return HAL_ERROR;

	HAL_StatusTypeDef retVal = HAL_OK;

	// decide what to do next depending on the function code
	switch (mf->code)
	{
		case e_mbFuncCode_ReadHoldingRegisters:
		{
			// check if byte count equals No. of registers
			uint16_t noOfRegs = (mbm_sentFrame.data[2] << 8) | mbm_sentFrame.data[3];
			if (((uint16_t)mf->data[0] / 2) != noOfRegs)
				goto badData;
			else
			{
				// crc stage
				*data_p = (uint8_t*)&mbm_rxFrames[mbm_rxFramesIndex].crc;
				*len = 2;
			}
			break;
		}

		default:
		{
			badData:
			// same principle as for 1st byte
			*data_p = &mbm_rxFrames[mbm_rxFramesIndex].code;
			*len = MBG_MAX_DATA_LEN + 3;
			retVal =  HAL_ERROR; // unknown function code
		}
	}

	return retVal;


}

/*
 * @brief	ModBus MASTER strong override function.
 * 			Use it to turn on the received after succesfull request is sent.
 * 			Return if \ref uHandle does not match configured periph.
 * @param	uHandle: pointer to the uart modbus slave struct.
 */
void mbm_uartTxRoutine(UART_HandleTypeDef* uHandle)
{
	// check either pointer is proper
	if (uHandle != mbm_uart.handle)
		return;

	// Enable receiver for the 1st address byte, no rx timeout
	mbg_EnableReceiver(mbm_uart.handle, &mbm_rxFrames[mbm_rxFramesIndex].addr, 1, 0);

	// set state
	mbm_rxState = e_mbsRxState_addr;

	// TODO turn on global rx timeout
}

/*
 * @brief	ModBus MASTER strong override function.
 * 			Use it to parse RESPONSES from the slave device.
 * 			Return if \ref uHandle does not match configured periph.
 * @param	uHandle: pointer to the uart modbus slave struct.
 */
void mbm_uartRxRoutine(UART_HandleTypeDef* uHandle)
{
	// check either pointer is proper
	if (uHandle != mbm_uart.handle)
		return;

	// define operational vars, set default values
	uint8_t* data_p = 0;
	uint32_t len = 0;
	bool rxTimeoutSet = true;

	// enter state machine and comlete the modbus request frame
	switch (mbm_rxState)
	{
		case e_mbsRxState_addr: // Obtained slave address, check either it matches
		{
			if (mbm_rxFrames[mbm_rxFramesIndex].addr == mbm_sentFrame.addr)
			{
				// request adressed to me, get the function code
				data_p = &mbm_rxFrames[mbm_rxFramesIndex].code;
				len = 1;
				mbm_rxState = e_mbsRxState_funcCode;
			}
			else // no match, wait for rx timeout
			{
				/*
				 * Gather dummy bytes from code member, just to keep rx DMA on.
				 * In theory this is safe, at max possible length data,
				 * function code and crc members should be filled.
				 */
				data_p = &mbm_rxFrames[mbm_rxFramesIndex].code;
				len = MBG_MAX_DATA_LEN + 3; // 3 for fcode + crc at max length

				// yield slave address error
				mbm_RespParseErrorSlaveAddr(mbm_rxFrames[mbm_rxFramesIndex].addr);
			}
			break;
		}

		case e_mbsRxState_funcCode: // obtained function code
		{
			// if the function is known, go further. If not, start from the begging
			// after timeout. Allow user to handle the error.
			if (!mbm_ExamineFuncCode(mbm_rxFrames[mbm_rxFramesIndex].code, &data_p, &len))
				mbm_rxState = e_mbsRxState_data;
			else // yield function code error
				mbm_RespParseErrorFuncCode(mbm_rxFrames[mbm_rxFramesIndex].code);
			break;
		}

		case e_mbsRxState_data: // received the data
		{
			// check data. If its ok go further
			if (!mbm_ExamineData(&mbm_rxFrames[mbm_rxFramesIndex], &data_p, &len))
				mbm_rxState = e_mbsRxState_crc; // receive 16 bit CRC
			else
				mbm_RespParseErrorData(mbm_rxFrames[mbm_rxFramesIndex].data,
						mbm_rxFrames[mbm_rxFramesIndex].dataLen);
			break;
		}

		case e_mbsRxState_crc:
		{
			// Add a completed frame to the queue, crc will be calculated there
			// then wait for the rx timeout to restart the receiver

			if (osOK == osMessagePut(mbm_msgQId_rX,
					(uint32_t)&mbm_rxFrames[mbm_rxFramesIndex], 0))
			{
				if (++mbm_rxFramesIndex >= MBM_RCV_QUEUE_SIZE)
					mbm_rxFramesIndex = 0;
			}

			// set default state
			mbm_rxState = e_mbsRxState_none;
			break;
		}

		default: // unknown state, wait for addr byte without timeut
		{
			data_p = &mbm_rxFrames[mbm_rxFramesIndex].addr;
			len = 1;
			rxTimeoutSet = false;
			break;
		}
	}

	// Wait for set data if operational vars set correctly
	if (len && data_p)
		mbg_EnableReceiver(uHandle, data_p, len, rxTimeoutSet);
	else
		mbm_RespParseError(); // yield generic parsing error
}

/*
 * @brief	Dequeue task for handling ready MODBUS responses tied together in
 * 			the uart RX interrupt routine.
 */
void mbm_task_rxDequeue(void const* argument)
{
	osEvent retEvent;
	mbgFrame_t* mf;

	while (1)
	{
		/*
		 * 	Peek is used instead of get, in order to keep the message in the queue
		 *	for the time of response creation and sending. the message gas to be
		 *	dequeued eventually after response is sent.
		 */
		retEvent = osMessagePeek(mbm_msgQId_rX, osWaitForever);

		if (retEvent.status == osEventMessage)
		{
			mf = (mbgFrame_t*)retEvent.value.p;

			// validate crc
			if (!mbg_CheckCrc(mf))
			{
				// crc OK, proceed. Check function code
				switch (mf->code)
				{
					case e_mbFuncCode_ReadHoldingRegisters:
					{
						mbs_CheckReadHoldingRegistersRequest(mf);
						break;
					}

					/*
					 * allow user to decide what to do on bad function code
					 * It should never come to this however, as function code
					 * is checked earlier in the parsing mechanism
					 */
					default:
					{

					}
				}
			}

			// finally remove the item from the queue. No need to check if there is
			// any message, because this thread is the only dequeuer.
			osMessageGet(mbm_msgQId_rX, osWaitForever);
		}
	}
}

/*
 * @brief	ModBus MASTER strong override function.
 * 			Use it to parse receiver timeouts.
 * 			Return if \ref uHandle does not match configured periph.
 * @param	uHandle: pointer to the uart modbus master struct.
 */
void mbm_uartRxTimeoutRoutine(UART_HandleTypeDef* uHandle)
{
	// check either pointer is proper
	if (uHandle != mbm_uart.handle)
		return;

	// allow user to handle the timeout
	if (e_mbsRxState_none != mbm_rxState)
		mbm_RespRxTimeoutError(mbm_rxState, &mbm_rxFrames[mbm_rxFramesIndex]);

	// We are in t3.5 rx timeout routine,
	// meaning modbus frame has to be gathered from the beginning.

	// Enable receiver but without rx timeout, wait forever for the 1st address byte
	mbg_EnableReceiver(mbm_uart.handle, &mbm_rxFrames[mbm_rxFramesIndex].addr, 0, 0);

	// set rxState to the beginning
	mbm_rxState = e_mbsRxState_none;

	// clear awaiting slave addr
	mbm_sentFrame.addr = 0;

	// debug
	HAL_GPIO_TogglePin(GPIOA, 1 << 5);
}

// overrides

/*
 * @brief	Function is called during parsing response from slave if the slave address
 * 			of the response does not match the slave address of sent request.
 * @param	code: Wrong slave address.
 */
__attribute__((weak)) void mbm_RespParseErrorSlaveAddr(uint8_t addr) { }

/*
 * @brief	Function is called during parsing response from slave if the function code
 * 			of the response does not match the function code of sent request.
 * @param	code: Wrong function code.
 */
__attribute__((weak)) void mbm_RespParseErrorFuncCode(mbgExCode_t code) { }

/*
 * @brief	Function is called during parsing response from slave if the data part
 * 			of the response is not consistent with the master request.
 * @param	data: pointer to the data buffer.
 * @param	len: Length of the data buffer.
 */
__attribute__((weak)) void mbm_RespParseErrorData(const uint8_t* const data,
												  uint16_t len) { }

/*
 * @brief	Function is called during parsing response from slave whenever any error
 * 			during parsing occurs.
 */
__attribute__((weak)) void mbm_RespParseError() { }

/*
 * @brief	Function is called during parsing response from slave whenever rx timeout
 * 			occurs.
 * @param	state: machine state of the response parser when rx timeout occured.
 * @param	mf: pointer to the gathered modbus frame
 */
__attribute__((weak)) void mbm_RespRxTimeoutError(mbgRxState_t state,
												  const mbgFrame_t* const mf) { }

/*
 * @brief	The default read holding registers response function.
 * @param	mf: pointer to a modbus frame struct.
 */
__attribute__((weak)) void mbs_CheckReadHoldingRegistersRequest(
		const mbgFrame_t* const mf) { }














