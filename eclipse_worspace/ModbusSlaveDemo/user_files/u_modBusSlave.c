/*
 * u_modBusSlave.c
 *
 *  Created on: 14 lis 2016
 *      Author: Lukasz
 */



/* Includes ------------------------------------------------------------------*/
#include "u_modBusSlave.h"

/* Defines and macros --------------------------------------------------------*/

/* Enums and structs ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

static mbsUart_t* mbs_u[MBS_MAX_NO_OF_SLAVES];
static uint32_t mbs_slavesNr;

/* Public variables ----------------------------------------------------------*/

/* Fuction prototypes --------------------------------------------------------*/
void mbs_task_rxDequeue(void const* argument);
HAL_StatusTypeDef mbs_ExamineFuncCode(mbsUart_t* mbsu, uint8_t** data_p, uint32_t* len);
HAL_StatusTypeDef mbs_ExamineDataLen(mbsUart_t* mbsu, uint8_t** data_p, uint32_t* len);
HAL_StatusTypeDef mbs_SendErrorResponse(mbgUart_t* uart, mbgFrame_t* mf, mbgExCode_t exCode);
mbsUart_t* mbs_GetModuleFromUartHandle(UART_HandleTypeDef* uart);

/* Function declarations -----------------------------------------------------*/

/*
 * @brief	Initializes the MODBUS Slave controller.
 * @param	mbsu: pointer to modbus slave uart handle
 * @param	noOfModules: number of modules to initialize (minimum 1)
 * @return	HAL_OK if initialization was succesfull.
 */
HAL_StatusTypeDef mbs_Init(mbsUart_t* mbsu, size_t noOfModules)
{
	// check if modules number is not zero
	if (!noOfModules)
		noOfModules = 1;

	HAL_StatusTypeDef retVal = HAL_OK;
	mbs_slavesNr = noOfModules;
	mbsUart_t* m = 0;
	osMessageQDef_t msgDef_temp;

	// threads definitions
	osThreadDef(mbsRxTask, mbs_task_rxDequeue, osPriorityAboveNormal, noOfModules, 256);

	// do everything for each module
	for (size_t i = 0; i < noOfModules; i++)
	{
		// check mbsu pointers
		if (!(mbsu + i))
			return HAL_ERROR;

		// Convenience
		m = mbsu + i;

		// validate slave address
		if ((m->slaveAddr < MBS_MIN_ADDR) || (m->slaveAddr > MBS_MAX_ADDR))
			return HAL_ERROR;

		// assign parameter variables
		mbs_u[i] = m;

		// init the generic uart module
		retVal += mbg_UartInit(&m->mbg);

		// create master requests reception queue.
		// In theory one item queue should be enough.
		// Add some buffer just to be sure.
		msgDef_temp.item_sz = sizeof(mbgFrame_t*);

		// make sure at least one item queue will be created
		if (!m->mbg.rxQ.framesBufLen)
			m->mbg.rxQ.framesBufLen = 1;
		msgDef_temp.queue_sz = m->mbg.rxQ.framesBufLen;

		// create the queue
		m->mbg.rxQ.msgQId_rX = osMessageCreate(&msgDef_temp, NULL);
		if (!m->mbg.rxQ.msgQId_rX)
			retVal++;

		// create thread for dequeuing
		m->mbg.rxQ.sysId_rX = osThreadCreate(osThread(mbsRxTask), m);
		if (!m->mbg.rxQ.sysId_rX)
			retVal++;

		// defaulr rx state
		m->mbg.rxState = e_mbsRxState_addr;
	}

	return retVal;
}

/*
 * @brief	If \ref uart is found in one of the slave structs, the slave struct
 * 			in which it was found is returned. Otherwised zero is returned.
 * @param	uart: handle
 * @return	modbus slave uart module pointer or zero.
 */
mbsUart_t* mbs_GetModuleFromUartHandle(UART_HandleTypeDef* uart)
{
	if (!uart)
		return 0;

	for (size_t i = 0; i < mbs_slavesNr; i++)
	{
		if (uart == mbs_u[i]->mbg.handle)
			return mbs_u[i];
	}

	return 0;
}

/*
 * @brief	Call this method in function code examination routine.
 * @param	mbsu: modbus slave uart module pointer.
 * @param	data_p: data pointer under which rx data will be saved,
 * @param	len: length of received data.
 * @return	HAL_ERROR if parameters are wrong or when request is unknown
 */
HAL_StatusTypeDef mbs_ExamineFuncCode(mbsUart_t* mbsu, uint8_t** data_p, uint32_t* len)
{
	if (!mbsu || !data_p || !len)
		return HAL_ERROR;

	// assume known frame
	mbsu->mbg.rxState = e_mbsRxState_data;
	*data_p = mbsu->mbg.rxQ.frames[mbsu->mbg.rxQ.framesIndex].data;
	HAL_StatusTypeDef retVal = HAL_OK;

	// decide what to do next depending on the function code
	switch (mbsu->mbg.rxQ.frames[mbsu->mbg.rxQ.framesIndex].code)
	{
		case e_mbFuncCode_WriteSingleCoil:
		case e_mbFuncCode_ReadCoils:
		case e_mbFuncCode_ReadHoldingRegisters:
		{
			// For this request one has to wait for 4 bytes:
			// Starting address (2), quantity of registers/ coils/ Output Value  (2)
			*len = 4;
			break;
		}

		case e_mbFuncCode_WriteMultipleCoils:
		case e_mbFuncCode_WriteMultipleRegisters:
		{
			// Wait for 5 bytes:
			// Starting address (2), Quantity of Outputs (2), Byte Count (1)
			*len = 5;
			break;
		}

		default:
		{
			// same principle as for 1st byte
			*data_p = &mbsu->mbg.rxQ.frames[mbsu->mbg.rxQ.framesIndex].code;
			*len = MBG_MAX_DATA_LEN + 3;
			mbsu->mbg.rxState =  e_mbsRxState_none; // unknown function code
			retVal = HAL_ERROR;
		}
	}

	// save length for generic purposes, like crc calculation
	mbsu->mbg.rxQ.frames[mbsu->mbg.rxQ.framesIndex].dataLen = *len;

	return retVal;
}

/*
 * @brief	Call this method in data length examination routine.
 * @param	mbsu: modbus slave uart module pointer.
 * @param	data_p: data pointer under which rx data will be saved,
 * @param	len: length of received data.
 * @return	HAL_ERROR if parameters are wrong or when request is unknown
 */
HAL_StatusTypeDef mbs_ExamineDataLen(mbsUart_t* mbsu, uint8_t** data_p, uint32_t* len)
{
	if (!mbsu || !data_p || !len)
		return HAL_ERROR;

	mbsu->mbg.rxState = e_mbsRxState_none;
	HAL_StatusTypeDef retVal = HAL_ERROR;

	// decide what to do next depending on the function code
	switch (mbsu->mbg.rxQ.frames[mbsu->mbg.rxQ.framesIndex].code)
	{
		case e_mbFuncCode_WriteSingleCoil:
		case e_mbFuncCode_ReadCoils:
		case e_mbFuncCode_ReadHoldingRegisters:
		{
			// Next 2 bytes will be the CRC
			*data_p = (uint8_t*)&mbsu->mbg.rxQ.frames[mbsu->mbg.rxQ.framesIndex].crc;
			*len = 2;
			mbsu->mbg.rxState = e_mbsRxState_crc;
			retVal = HAL_OK;
			break;
		}

		case e_mbFuncCode_WriteMultipleCoils:
		case e_mbFuncCode_WriteMultipleRegisters:
		{
			// set pointer to 5th element
			*data_p = mbsu->mbg.rxQ.frames[mbsu->mbg.rxQ.framesIndex].data + 5;

			// Last data byte holds the amount of bytes to receive
			*len = mbsu->mbg.rxQ.frames[mbsu->mbg.rxQ.framesIndex].data[4];
			mbsu->mbg.rxState = e_mbsRxState_data2;
			retVal = HAL_OK;

			// update length
			mbsu->mbg.rxQ.frames[mbsu->mbg.rxQ.framesIndex].dataLen += *len;
			break;
		}

		default:
		{
			// same principle as for 1st byte
			*data_p = &mbsu->mbg.rxQ.frames[mbsu->mbg.rxQ.framesIndex].code;
			*len = MBG_MAX_DATA_LEN + 3;
		}
	}

	return retVal;
}

/*
 * @brief	Send an exception code frame to the master in response to a request.
 * @param	uart: uart handler struct pointer.
 * @param	mf: pointer to a received modbus frame, that will now be used as data
 * 			container for creating a response frame. This is done like this due to
 * 			memory and speed optimalisation reasons.
 * 			The protocol is half duplex so its safe.
 * @param	exCode: Exception code of type \ref mbsExCode_t.
 * @return
 */
HAL_StatusTypeDef mbs_SendErrorResponse(mbgUart_t* uart, mbgFrame_t* mf, mbgExCode_t exCode)
{
	if (!uart->handle || !mf)
		return HAL_ERROR;

	// modify function code (becomes error code)
	mf->code |= MBS_EXCODE_ADDR_OR;

	// save exception code in data field
	mf->data[0] = (uint8_t)exCode;

	// update data length
	mf->dataLen = 1;

	// send the data and return
	return mbg_SendFrame(uart, mf);
}

/*
 * @brief	ModBus SLAVE strong override function.
 * 			Use it to parse REQUESTS from the master device.
 * 			Return if \ref uHandle does not match configured periph.
 * @param	uHandle: pointer to the uart modbus slave struct.
 */
void mbs_uartRxRoutine(UART_HandleTypeDef* uHandle)
{
	// find which module triggered the interrupt
	mbsUart_t* mbsu = mbs_GetModuleFromUartHandle(uHandle);
	if (!mbsu)
		return; // unknown module

	// define operational vars, set default values
	uint8_t* data_p = 0;
	uint32_t len = 0;
	bool rxTimeoutSet = true;

	// enter state machine and comlete the modbus request frame
	switch (mbsu->mbg.rxState)
	{
		case e_mbsRxState_addr: // Obtained slave address, check either it matches
		{
			// next point no matter what
			data_p = &mbsu->mbg.rxQ.frames[mbsu->mbg.rxQ.framesIndex].code;

			if (mbsu->mbg.rxQ.frames[mbsu->mbg.rxQ.framesIndex].addr == mbsu->slaveAddr)
			{
				// request adressed to me, get the function code
				len = 1;
				mbsu->mbg.rxState = e_mbsRxState_funcCode;
			}
			else // no match, wait for rx timeout
			{
				/*
				 * Gather dummy bytes from code member, just to keep rx DMA on.
				 * In theory this is safe, at max possible length data,
				 * function code and crc members should be filled.
				 */
				len = MBG_MAX_DATA_LEN + 3; // 3 for fcode + crc at max length
			}
			break;
		}

		case e_mbsRxState_funcCode: // obtained function code
		{
			// if the function is known, go further. If not, start from the begging
			// after timeout
			mbs_ExamineFuncCode(mbsu, &data_p, &len);
			break;
		}

		case e_mbsRxState_data: // received the data (length/ registers)
		{
			// Handle the data more further. Either receive more or wait for CRC
			mbs_ExamineDataLen(mbsu, &data_p, &len);
			break;
		}

		case e_mbsRxState_data2: // receive the data (registers)
		{
			// Next 2 bytes will be the CRC
			data_p = (uint8_t*)&mbsu->mbg.rxQ.frames[mbsu->mbg.rxQ.framesIndex].crc;
			len = 2;
			mbsu->mbg.rxState = e_mbsRxState_crc;
			break;
		}

		case e_mbsRxState_crc:
		{
			// Add a completed frame to the queue, crc will be calculated there
			// then wait for the rx timeout to restart the receiver

			if (osOK == osMessagePut(mbsu->mbg.rxQ.msgQId_rX,
					(uint32_t)&mbsu->mbg.rxQ.frames[mbsu->mbg.rxQ.framesIndex], 0))
			{
				if (++mbsu->mbg.rxQ.framesIndex >= mbsu->mbg.rxQ.framesBufLen)
					mbsu->mbg.rxQ.framesIndex = 0;
			}

			mbsu->mbg.rxState = e_mbsRxState_none;
			break;
		}

		default: // unknown state, wait for addr byte without timeout
		{
			data_p = &mbsu->mbg.rxQ.frames[mbsu->mbg.rxQ.framesIndex].addr;
			len = 1;
			rxTimeoutSet = false;
			break;
		}
	}

	// Wait for set data if operational vars set correctly
	if (len && data_p)
		mbg_EnableReceiver(uHandle, data_p, len, rxTimeoutSet);
}

/*
 * @brief	ModBus SLAVE strong override function.
 * 			Use it to turn on the receiver after succesfull response is sent.
 * 			Return if \ref uHandle does not match configured periph.
 * @param	uHandle: pointer to the uart modbus slave struct.
 */
void mbs_uartTxRoutine(UART_HandleTypeDef* uHandle)
{
	mbs_uartRxTimeoutRoutine(uHandle);
}

/*
 * @brief	ModBus SLAVE strong override function.
 * 			Use it to parse receiver timeouts.
 * 			Return if \ref uHandle does not match configured periph.
 * @param	uHandle: pointer to the uart modbus slave struct.
 */
void mbs_uartRxTimeoutRoutine(UART_HandleTypeDef* uHandle)
{
	// find module (check either pointer is proper is inside)
	mbsUart_t* mbsu = mbs_GetModuleFromUartHandle(uHandle);
	if (!mbsu)
		return;

	// We are in t3.5 rx timeout routine,
	// meaning modbus frame has to be gathered from the beginning.

	// Enable receiver but without rx timeout, wait forever for the 1st address byte
	mbg_EnableReceiver(mbsu->mbg.handle,
			&mbsu->mbg.rxQ.frames[mbsu->mbg.rxQ.framesIndex].addr, 1, 0);

	// set rxState to the beginning
	mbsu->mbg.rxState = e_mbsRxState_addr;
}

/*
 * @brief	Dequeue task for handling ready MODBUS requests tied together in
 * 			the uart RX interrupt routine.
 */
void mbs_task_rxDequeue(void const* argument)
{
	if (!argument)
		return;

	mbsUart_t* mbsu = (mbsUart_t*)argument;

	// Initially enable receiver for the 1st address byte, no rx timeout
	mbs_uartRxTimeoutRoutine(mbsu->mbg.handle);

	osEvent retEvent;
	mbgExCode_t exCode;
	mbgFrame_t* mf;

	while (1)
	{
		/*
		 * 	Peek is used instead of get, in order to keep the message in the queue
		 *	for the time of response creation and sending. the message has to be
		 *	dequeued eventually after response is sent.
		 */
		retEvent = osMessagePeek(mbsu->mbg.rxQ.msgQId_rX, osWaitForever);

		if (retEvent.status == osEventMessage)
		{
			mf = (mbgFrame_t*)retEvent.value.p;

			// validate crc
			if (!mbg_CheckCrc(mf, 0))
			{
				// crc OK, proceed. Check function code
				switch (mf->code)
				{
					case e_mbFuncCode_ReadCoils:
					{
						exCode = mbs_CheckReadCoils(mf);
						break;
					}

					case e_mbFuncCode_ReadHoldingRegisters:
					{
						exCode = mbs_CheckReadHoldingRegisters(mbsu, mf);
						break;
					}

					case e_mbFuncCode_WriteSingleCoil:
					{
						exCode = mbs_CheckWriteSingleCoil(mf);
						break;
					}

					// for unknown function ILLEGAL FUNCTION error answer code
					// has to be sent
					default: exCode = e_mbsExCode_illegalFunction;
				}

				// if exception code set, send error response
				if (exCode)
					mbs_SendErrorResponse(&mbsu->mbg, mf, exCode);

				// otherwise standard response.
				// Override func has to fill mf with data.
				else
					mbg_SendFrame(&mbsu->mbg, mf);
			}

			// finally remove the item from the queue. No need to check if there is
			// any message, because this thread is the only dequeuer.
			osMessageGet(mbsu->mbg.rxQ.msgQId_rX, osWaitForever);
		}
	}
}

// overrides

/*
 * @brief	The default read holding registers function.
 * 			If not overriden, it always returns illegar data address ex code.
 * @param	mf: pointer to a modbus frame struct.
 * @return  exception code, 0 if function executed properly.
 */
__attribute__((weak)) mbgExCode_t mbs_CheckReadHoldingRegisters(const mbsUart_t* const mbs,
		mbgFrame_t* mf)
{
	return e_mbsExCode_illegalDataAddr;
}

/*
 * @brief	The default read coils function.
 * 			If not overriden, it always returns illegar data address ex code.
 * @param	mf: pointer to a modbus frame struct.
 * @return  exception code, 0 if function executed properly.
 */
__attribute__((weak)) mbgExCode_t mbs_CheckReadCoils(mbgFrame_t* mf)
{
	return e_mbsExCode_illegalDataAddr;
}

/*
 * @brief	The default 05 (0x05) Write Single Coil function.
 * 			If not overriden, it always returns illegar data address ex code.
 * @param	mf: pointer to a modbus frame struct.
 * @return  exception code, 0 if function executed properly.
 */
__attribute__((weak)) mbgExCode_t mbs_CheckWriteSingleCoil(mbgFrame_t* mf)
{
	return e_mbsExCode_illegalDataAddr;
}







