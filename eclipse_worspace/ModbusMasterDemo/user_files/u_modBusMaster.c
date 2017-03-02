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

static mbmUart_t* mbm_u[MBS_MAX_NO_OF_MASTERS];
static uint32_t mbm_mastersNr;

/* Public variables ----------------------------------------------------------*/

/* Fuction prototypes --------------------------------------------------------*/
void mbm_task_rxDequeue(void const* argument);
void mbm_task_rxTimeout(void const* argument);
HAL_StatusTypeDef mbm_ExamineFuncCode(mbmUart_t* mbmu, uint8_t** data_p, uint32_t* len);
HAL_StatusTypeDef mbm_ExamineData(mbmUart_t* mbmu, uint8_t** data_p, uint32_t* len);
HAL_StatusTypeDef mbm_RespTimeoutRestart(mbmUart_t* mbmu);
HAL_StatusTypeDef mbm_RespTimeoutStop(mbmUart_t* mbmu);
void mbm_RespTimeoutAction(mbmUart_t* mbmu);
mbmUart_t* mbm_GetModuleFromUartHandle(UART_HandleTypeDef* uart);

/* Function declarations -----------------------------------------------------*/

/*
 * @brief	Initializes the MODBUS Master controller.
 * @param	mbmu: pointer to modbus master uart handle
 * @param	noOfModules: number of modules to initialize (minimum 1)
 * @return	HAL_OK if initialization was succesfull.
 */
HAL_StatusTypeDef mbm_Init(mbmUart_t* mbmu, size_t noOfModules)
{
	// check if modules number is not zero
	if (!noOfModules)
		noOfModules = 1;

	HAL_StatusTypeDef retVal = HAL_OK;
	mbm_mastersNr = noOfModules;
	mbmUart_t* m = 0;
	osMessageQDef_t msgDef_temp;

	// threads definitions
	osThreadDef(mbmRxTask, mbm_task_rxDequeue, osPriorityAboveNormal, noOfModules, 128);
	osThreadDef(mbmRxTimTask, mbm_task_rxTimeout, osPriorityHigh, noOfModules, 128);

	// do everything for each module
	for (size_t i = 0; i < noOfModules; i++)
	{
		// Convenience
		m = mbmu + i;

		// check mbsu pointers
		if (!(mbmu + i))
			return HAL_ERROR;

		// assign parameter variables
		mbm_u[i] = m;

		// init the generic uart module
		retVal += mbg_UartInit(&m->mbg);

		// create slave responses reception queue.
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
		m->mbg.rxQ.sysId_rX = osThreadCreate(osThread(mbmRxTask), m);
		if (!m->mbg.rxQ.sysId_rX)
			retVal++;

		// create slave response timeout queue
		msgDef_temp.item_sz = sizeof(uint32_t);
		msgDef_temp.queue_sz = 1;

		// create the queue
		m->toutQ.msgQId_rxTimeout = osMessageCreate(&msgDef_temp, NULL);
		if (!m->toutQ.msgQId_rxTimeout)
			retVal++;

		// create thread for dequeuing
		m->toutQ.sysId_rxTimeout = osThreadCreate(osThread(mbmRxTimTask), m);
		if (!m->toutQ.sysId_rxTimeout)
			retVal++;

		// default rx state
		m->mbg.rxState = e_mbsRxState_none;
	}

	return retVal;
}

/*
 * @brief	If \ref uart is found in one of the master structs, the master struct
 * 			in which it was found is returned. Otherwised zero is returned.
 * @param	uart: handle
 * @return	modbus master uart module pointer or zero.
 */
mbmUart_t* mbm_GetModuleFromUartHandle(UART_HandleTypeDef* uart)
{
	if (!uart)
		return 0;

	for (size_t i = 0; i < mbm_mastersNr; i++)
	{
		if (uart == mbm_u[i]->mbg.handle)
			return mbm_u[i];
	}

	return 0;
}

/*
 * @brief	Read holding registers request command.
 * @param	slaveAddr: Slave to which the request is addressed to.
 * @param	startAddr: Address of the register the reading starts from.
 * @param	nrOfRegs: Number of 16 bit registers to read, starting from \ref startAddr.
 * @param	HAL_OK if request sent succesfully.
 */
HAL_StatusTypeDef mbm_RequestReadHoldingRegs(mbmUart_t* mbmu,
									  uint8_t slaveAddr,
									  uint16_t startAddr,
									  uint16_t nrOfRegs)
{
	if (!nrOfRegs)
		return HAL_OK; // nothing to read

	assert_param(mbmu);
	if(!mbmu)
		return HAL_ERROR;

	// set slave addr
	mbmu->sendFrame.addr = slaveAddr;

	// set function code
	mbmu->sendFrame.code = e_mbFuncCode_ReadHoldingRegisters;

	// starting address HI and LO
	mbmu->sendFrame.dataLen = 0;
	mbmu->sendFrame.data[mbmu->sendFrame.dataLen++] = (uint8_t)(startAddr >> 8);
	mbmu->sendFrame.data[mbmu->sendFrame.dataLen++] = (uint8_t)startAddr;

	// quantity of registers HI and LO
	mbmu->sendFrame.data[mbmu->sendFrame.dataLen++] = (uint8_t)(nrOfRegs >> 8);
	mbmu->sendFrame.data[mbmu->sendFrame.dataLen++] = (uint8_t)nrOfRegs;

	// msg type for printing
	mbmu->sendFrame.msgType = e_mbgMesgType_Request;

	// send
	if (mbg_SendFrame(&mbmu->mbg, &mbmu->sendFrame))
	{
		mbmu->sendFrame.addr = 0;
		return HAL_ERROR;
	}

	return HAL_OK;
}

/*
 * @brief	Write single coil request command.
 * @param	slaveAddr: Slave to which the request is addressed to.
 * @param	startAddr: Address of the register the reading starts from.
 * @param	state: non zero sets the coil, 0 clears it
 * @param	HAL_OK if request sent succesfully.
 */
HAL_StatusTypeDef mbm_RequestWriteSingleCoil(mbmUart_t* mbmu,
									  uint8_t slaveAddr,
									  uint16_t outputAddr,
									  uint8_t state)
{
	assert_param(mbmu);
	if(!mbmu)
		return HAL_ERROR;

	// set slave addr
	mbmu->sendFrame.addr = slaveAddr;

	// set function code
	mbmu->sendFrame.code = e_mbFuncCode_WriteSingleCoil;

	// starting address HI and LO
	mbmu->sendFrame.dataLen = 0;
	mbmu->sendFrame.data[mbmu->sendFrame.dataLen++] = (uint8_t)(outputAddr >> 8);
	mbmu->sendFrame.data[mbmu->sendFrame.dataLen++] = (uint8_t)outputAddr;

	// output value HI and LO
	mbmu->sendFrame.data[mbmu->sendFrame.dataLen++] = state ? 0xFF : 0;
	mbmu->sendFrame.data[mbmu->sendFrame.dataLen++] = 0;

	// msg type for printing
	mbmu->sendFrame.msgType = e_mbgMesgType_Request;

	// send
	if (mbg_SendFrame(&mbmu->mbg, &mbmu->sendFrame))
	{
		mbmu->sendFrame.addr = 0;
		return HAL_ERROR;
	}

	return HAL_OK;
}

/*
 * @brief	Call this method in function code examination routine.
 * @param	mbmu: modbus master uart module pointer.
 * @param	data_p: data pointer under which rx data will be saved,
 * @param	len: length of received data.
 * @return	HAL_ERROR if parameters are wrong or when request is unknown
 */
HAL_StatusTypeDef mbm_ExamineFuncCode(mbmUart_t* mbmu, uint8_t** data_p, uint32_t* len)
{
	if (!mbmu || !data_p || !len)
		return HAL_ERROR;

	HAL_StatusTypeDef retVal = HAL_OK;
	mbmu->mbg.rxState = e_mbsRxState_data;

	// decide what to do next depending on the function code
	switch (mbmu->mbg.rxQ.frames[mbmu->mbg.rxQ.framesIndex].code)
	{
		case e_mbFuncCode_ReadHoldingRegisters:
		{
			// Byte count (1), then number of registers read from sent frame
			*data_p = mbmu->mbg.rxQ.frames[mbmu->mbg.rxQ.framesIndex].data;
			uint16_t noOfRegs = (mbmu->sendFrame.data[2] << 8) | mbmu->sendFrame.data[3];
			*len = 1 + noOfRegs * 2; // 1 for byte count and *2 because regs are 16 bit

			// save length for generic purposes, like crc calculation
			mbmu->mbg.rxQ.frames[mbmu->mbg.rxQ.framesIndex].dataLen = *len;
			break;
		}

		case e_mbFuncCode_WriteSingleCoil:
		{
			*data_p = mbmu->mbg.rxQ.frames[mbmu->mbg.rxQ.framesIndex].data;
			*len = 6; // output adde HI+LO, output val HI+LO, CRC
			mbmu->mbg.rxQ.frames[mbmu->mbg.rxQ.framesIndex].dataLen = *len;
			mbmu->mbg.rxState = e_mbsRxState_crc;
			break;
		}

		default:
		{
			// same principle as for 1st byte
			*data_p = &mbmu->mbg.rxQ.frames[mbmu->mbg.rxQ.framesIndex].code;
			*len = MBG_MAX_DATA_LEN + 3;
			mbmu->mbg.rxState = e_mbsRxState_none;
			retVal =  HAL_ERROR; // unknown function code
		}
	}

	return retVal;
}

/*
 * @brief	Call this method in data examination routine.
 * @param	mbmu: modbus master uart module pointer.
 * @param	data_p: data pointer under which rx data will be saved,
 * @param	len: length of received data.
 * @return	HAL_ERROR if parameters are wrong or when request is unknown
 */
HAL_StatusTypeDef mbm_ExamineData(mbmUart_t* mbmu, uint8_t** data_p, uint32_t* len)
{
	if (!mbmu || !data_p || !len)
		return HAL_ERROR;

	HAL_StatusTypeDef retVal = HAL_OK;

	// decide what to do next depending on the function code
	switch (mbmu->mbg.rxQ.frames[mbmu->mbg.rxQ.framesIndex].code)
	{
		case e_mbFuncCode_ReadHoldingRegisters:
		{
			// check if byte count equals No. of registers
			uint16_t noOfRegs = (mbmu->sendFrame.data[2] << 8) | mbmu->sendFrame.data[3];
			if (((uint16_t)mbmu->mbg.rxQ.frames[mbmu->mbg.rxQ.framesIndex].data[0] / 2) !=
					noOfRegs)
				goto label_badData;
			else
			{
				// crc stage
				*data_p = (uint8_t*)&mbmu->mbg.rxQ.frames[mbmu->mbg.rxQ.framesIndex].crc;
				*len = 2;
				mbmu->mbg.rxState = e_mbsRxState_crc;
			}
			break;
		}

		default:
		{
			label_badData:
			// same principle as for 1st byte
			*data_p = &mbmu->mbg.rxQ.frames[mbmu->mbg.rxQ.framesIndex].code;
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
	// find which module triggered the interrupt
	mbmUart_t* mbmu = mbm_GetModuleFromUartHandle(uHandle);
	if (!mbmu)
		return; // unknown module

	// Enable receiver for the 1st address byte, no rx timeout
	mbg_EnableReceiver(mbmu->mbg.handle,
			&mbmu->mbg.rxQ.frames[mbmu->mbg.rxQ.framesIndex].addr, 1, 0);

	// set state
	mbmu->mbg.rxState = e_mbsRxState_addr;

	// start global timeout timer
	mbm_RespTimeoutRestart(mbmu);
}

/*
 * @brief	ModBus MASTER strong override function.
 * 			Use it to parse RESPONSES from the slave device.
 * 			Return if \ref uHandle does not match configured periph.
 * @param	uHandle: pointer to the uart modbus slave struct.
 */
void mbm_uartRxRoutine(UART_HandleTypeDef* uHandle)
{
	// find which module triggered the interrupt
	mbmUart_t* mbmu = mbm_GetModuleFromUartHandle(uHandle);
	if (!mbmu)
		return; // unknown module

	// define operational vars, set default values
	uint8_t* data_p = 0;
	uint32_t len = 0;
	bool rxTimeoutSet = true;

	// enter state machine and comlete the modbus response frame
	switch (mbmu->mbg.rxState)
	{
		case e_mbsRxState_addr: // Obtained slave address, check either it matches
		{
			// next point no matter what
			data_p = &mbmu->mbg.rxQ.frames[mbmu->mbg.rxQ.framesIndex].code;

			if (mbmu->mbg.rxQ.frames[mbmu->mbg.rxQ.framesIndex].addr == mbmu->sendFrame.addr)
			{
				// response slave addr matches requested one, get the function code
				len = 1;
				mbmu->mbg.rxState = e_mbsRxState_funcCode;
			}
			else // no match, wait for rx timeout
			{
				/*
				 * Gather dummy bytes from code member, just to keep rx DMA on.
				 * In theory this is safe, at max possible length data,
				 * function code and crc members should be filled.
				 */
				len = MBG_MAX_DATA_LEN + 3; // 3 for fcode + crc at max length

				// yield slave address error
				mbm_RespParseErrorSlaveAddr(
						mbmu->mbg.rxQ.frames[mbmu->mbg.rxQ.framesIndex].addr);
			}
			break;
		}

		case e_mbsRxState_funcCode: // obtained function code
		{
			// if the function is known, go further. If not, start from the begging
			// after timeout. Allow user to handle the error.
			if (mbm_ExamineFuncCode(mbmu, &data_p, &len))
				mbm_RespParseErrorFuncCode(
						mbmu->mbg.rxQ.frames[mbmu->mbg.rxQ.framesIndex].code);
			break;
		}

		case e_mbsRxState_data: // received the data
		{
			// check data. If its ok go further
			if (mbm_ExamineData(mbmu, &data_p, &len))
				mbm_RespParseErrorData(mbmu->mbg.rxQ.frames[mbmu->mbg.rxQ.framesIndex].data,
						mbmu->mbg.rxQ.frames[mbmu->mbg.rxQ.framesIndex].dataLen);
			break;
		}

		case e_mbsRxState_crc:
		{
			// Add a completed frame to the queue, crc will be calculated there
			// then wait for the rx timeout to restart the receiver

			if (osOK == osMessagePut(mbmu->mbg.rxQ.msgQId_rX,
					(uint32_t)&mbmu->mbg.rxQ.frames[mbmu->mbg.rxQ.framesIndex], 0))
			{
				if (++mbmu->mbg.rxQ.framesIndex >= mbmu->mbg.rxQ.framesBufLen)
					mbmu->mbg.rxQ.framesIndex = 0;
			}

			// set default state
			mbmu->mbg.rxState = e_mbsRxState_none;
			break;
		}

		default: // unknown state, wait for addr byte without timeut
		{
			data_p = &mbmu->mbg.rxQ.frames[mbmu->mbg.rxQ.framesIndex].addr;
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
	if (!argument)
		return;

	mbmUart_t* mbmu = (mbmUart_t*)argument;

	osEvent retEvent;
	mbgFrame_t* mf;
	uint16_t calcCrc;

	// temp
	//mbm_RequestReadHoldingRegs(mbmu, 5, 0, 1);

	while (1)
	{
		/*
		 * 	Peek is used instead of get, in order to keep the message in the queue
		 *	for the time of response creation and sending. the message gas to be
		 *	dequeued eventually after response is sent.
		 */
		retEvent = osMessagePeek(mbmu->mbg.rxQ.msgQId_rX, osWaitForever);

		// turn off response timeout timer
		mbm_RespTimeoutStop(mbmu);

		if (retEvent.status == osEventMessage)
		{
			mf = (mbgFrame_t*)retEvent.value.p;

			// validate crc
			if (!mbg_CheckCrc(mf, &calcCrc))
			{
				// crc OK, proceed. Check function code, print the frame
				mf->msgType = e_mbgMesgType_Response;
				mbg_uartPrintFrame(mf);

				switch (mf->code)
				{
					case e_mbFuncCode_ReadHoldingRegisters:
					{
						mbm_CheckReadHoldingRegisters(mbmu, mf);
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

			// CRC mismatch
			else
				mbm_RespCrcMatchError(mf->crc, calcCrc);

			// finally remove the item from the queue. No need to check if there is
			// any message, because this thread is the only dequeuer.
			osMessageGet(mbmu->mbg.rxQ.msgQId_rX, osWaitForever);
		}
	}
}

/*
 * @brief	Response timeout thread
 */
void mbm_task_rxTimeout(void const* argument)
{
	if (!argument)
		return;

	mbmUart_t* mbmu = (mbmUart_t*)argument;
	osEvent retEvent;
	uint32_t timeout_ms;

	while (1)
	{
		// first wait for timeout information (forever)
		retEvent = osMessageGet(mbmu->toutQ.msgQId_rxTimeout, osWaitForever);

		if (osEventMessage == retEvent.status)
		{
			// obtain the required timeout
			timeout_ms = retEvent.value.v;

			// waiting for timeout starts here
			label_respTimeoutWait:

			// wait for the required timeout
			retEvent = osMessageGet(mbmu->toutQ.msgQId_rxTimeout, timeout_ms);

			// timeout occured
			if (osEventTimeout == retEvent.status)
				mbm_RespTimeoutAction(mbmu);

			// timeout restart occured
			else if (retEvent.status == osEventMessage)
			{
				if (retEvent.value.v)
				{
					timeout_ms = retEvent.value.v;
					goto label_respTimeoutWait;
				}
			}
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
	// find module (check either pointer is proper is inside)
	mbmUart_t* mbmu = mbm_GetModuleFromUartHandle(uHandle);
	if (!mbmu)
		return;

	// stop the timeout timer
	mbm_RespTimeoutStop(mbmu);

	// call timeout action
	mbm_RespTimeoutAction(mbmu);
}

/*
 * @brief	Timeout action (global and modbus)
 * @param	mbmu: modbus master module pointer.
 */
void mbm_RespTimeoutAction(mbmUart_t* mbmu)
{
	assert_param(mbmu);

	// allow user to handle the timeout
	if (e_mbsRxState_none != mbmu->mbg.rxState)
		mbm_RespRxTimeoutError(mbmu->mbg.rxState,
				&mbmu->mbg.rxQ.frames[mbmu->mbg.rxQ.framesIndex]);

	// We are in t3.5 rx timeout routine,
	// meaning modbus frame has to be gathered from the beginning.

	// Enable receiver but without rx timeout, wait forever for the 1st address byte
	mbg_EnableReceiver(mbmu->mbg.handle,
			&mbmu->mbg.rxQ.frames[mbmu->mbg.rxQ.framesIndex].addr, 0, 0);

	// set rxState to the beginning
	mbmu->mbg.rxState = e_mbsRxState_none;

	// clear awaiting slave addr
	mbmu->mbg.rxQ.frames[mbmu->mbg.rxQ.framesIndex].addr = 0;

	// debug
	//HAL_GPIO_TogglePin(GPIOA, 1 << 5);
}

/*
 * @brief	(Re)Starts the response timeout timer.
 * @param	mbmu: Modbus master module pointer
 */
HAL_StatusTypeDef mbm_RespTimeoutRestart(mbmUart_t* mbmu)
{
	assert_param(mbmu);

	return (HAL_StatusTypeDef)osMessagePut(
			mbmu->toutQ.msgQId_rxTimeout, mbmu->toutQ.respTimeout, osWaitForever);
}

/*
 * @brief	Stop the response timeout timer.
 * @param	mbmu: Modbus master module pointer
 */
HAL_StatusTypeDef mbm_RespTimeoutStop(mbmUart_t* mbmu)
{
	assert_param(mbmu);

	return (HAL_StatusTypeDef)osMessagePut(
			mbmu->toutQ.msgQId_rxTimeout, 0, osWaitForever);
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
 * @brief	Function is called when the response frame crc doesnt match the calculated one.
 * @param	rcvCrc: The crc that came with the response.
 * @param	calcCrc: Calculated crc.
 */
__attribute__((weak)) void mbm_RespCrcMatchError(uint16_t rcvCrc, uint16_t calcCrc) { }

/*
 * @brief	The default read holding registers response function.
 * @param	mbm: master module pointer.
 * @param	mf: pointer to a modbus frame struct.
 */
__attribute__((weak)) void mbm_CheckReadHoldingRegisters(const mbmUart_t* const mbm,
		const mbgFrame_t* const mf) { }














