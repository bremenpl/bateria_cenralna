/*
 * u_modBusMaster.c
 *
 *  Created on: 02.03.2017
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
mbmUart_t* mbm_GetModuleFromUart(UART_HandleTypeDef* uart);
mbmUart_t* mbm_GetModuleFromTimer(TIM_HandleTypeDef* timer);
mbmUart_t* mbm_GetModuleFromMbg(mbgUart_t* mbg);
void mbm_digForFrames(mbmUart_t* m, const uint8_t byte);
void mbm_rxFrameHandle(mbmUart_t* const mbmu);
HAL_StatusTypeDef mbm_RespTimeoutRestart(mbmUart_t* mbmu);
HAL_StatusTypeDef mbm_RespTimeoutStop(mbmUart_t* mbmu);

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
	osThreadDef(mbmRxTask, mbm_task_rxDequeue, osPriorityAboveNormal, noOfModules, 256);
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
		msgDef_temp.item_sz = sizeof(uint32_t);
		msgDef_temp.queue_sz = MBG_RECV_QUEUE_LEN;

		// create the queue
		m->mbg.rxQ.msgQId = osMessageCreate(&msgDef_temp, NULL);
		if (!m->mbg.rxQ.msgQId)
			retVal++;

		// create thread for dequeuing
		m->mbg.rxQ.sysId = osThreadCreate(osThread(mbmRxTask), &m->mbg);
		if (!m->mbg.rxQ.sysId)
			retVal++;

		// create slave response timeout queue
		msgDef_temp.item_sz = sizeof(uint32_t);
		msgDef_temp.queue_sz = 1;

		// create the queue
		m->toutQ.msgQId = osMessageCreate(&msgDef_temp, NULL);
		if (!m->toutQ.msgQId)
			retVal++;

		// create thread for dequeuing
		m->toutQ.sysId = osThreadCreate(osThread(mbmRxTimTask), m);
		if (!m->toutQ.sysId)
			retVal++;

		// default rx state
		m->mbg.rxState = e_mbgRxState_none;
	}

	return retVal;
}

/*
 * @brief	If \ref uart is found in one of the master structs, the master struct
 * 			in which it was found is returned. Otherwised zero is returned.
 * @param	uart: handle
 * @return	modbus master uart module pointer or zero.
 */
mbmUart_t* mbm_GetModuleFromUart(UART_HandleTypeDef* uart)
{
	assert_param(uart);

	for (size_t i = 0; i < mbm_mastersNr; i++)
	{
		if (uart == mbm_u[i]->mbg.handle)
			return mbm_u[i];
	}

	return 0;
}

/*
 * @brief	If \ref timer is found in one of the slave structs, the master struct
 * 			in which it was found is returned. Otherwised zero is returned.
 * @param	timer: handle
 * @return	modbus slave timer module pointer or zero.
 */
mbmUart_t* mbm_GetModuleFromTimer(TIM_HandleTypeDef* timer)
{
	assert_param(timer);

	for (size_t i = 0; i < mbm_mastersNr; i++)
	{
		if (timer == mbm_u[i]->mbg.rxQ.toutTim)
			return mbm_u[i];
	}

	return 0;
}

/*
 * @brief	If \ref mbg is found in one of the master structs, the master struct
 * 			in which it was found is returned. Otherwise zero is returned.
 * @param	timer: handle
 * @return	modbus generic module pointer or zero.
 */
mbmUart_t* mbm_GetModuleFromMbg(mbgUart_t* mbg)
{
	assert_param(mbg);

	for (size_t i = 0; i < mbm_mastersNr; i++)
	{
		if (mbg == &mbm_u[i]->mbg)
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
 * @brief	Response timeout thread
 */
void mbm_task_rxTimeout(void const* argument)
{
	assert_param(argument);

	mbmUart_t* mbmu = (mbmUart_t*)argument;
	osEvent retEvent;
	uint32_t timeout_ms;

	while (1)
	{
		// first wait for timeout information (forever)
		retEvent = osMessageGet(mbmu->toutQ.msgQId, osWaitForever);

		if (osEventMessage == retEvent.status)
		{
			// obtain the required timeout
			timeout_ms = retEvent.value.v;

			// waiting for timeout starts here
			label_respTimeoutWait:

			// wait for the required timeout
			retEvent = osMessageGet(mbmu->toutQ.msgQId, timeout_ms);

			// timeout occured
			if (osEventTimeout == retEvent.status)
				mbm_uartRxTimeoutRoutine(mbmu->mbg.rxQ.toutTim);

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
 * @brief	Rx dequeue task. Raw bytes received by master are examined here.
 * @param	argument: Pass mbg pointer struct here.
 */
void mbm_task_rxDequeue(void const* argument)
{
	assert_param(argument);

	mbmUart_t* m = mbm_GetModuleFromMbg((mbgUart_t*)argument);
	osEvent retEvent;

	while (1)
	{
		retEvent = osMessageGet(m->mbg.rxQ.msgQId, osWaitForever);

		if (retEvent.status == osEventMessage)
			mbm_digForFrames(m, (uint8_t)retEvent.value.v);
	}
}

/*
 * @brief	Run this function each time new byte arrives. It examines each byte (while
 * 			remembering the state) and takes actions if the frame is complete. this
 * 			function should not be run inside an interrupt.
 */
void mbm_digForFrames(mbmUart_t* m, const uint8_t byte)
{
	assert_param(m);
	assert_param(!mbg_inHandlerMode());

	switch (m->mbg.rxState)
	{
		case e_mbgRxState_addr:
		{
			m->mbg.rxFrame.addr = byte;

			// check if there is a match with request addr
			m->mbg.rxState = (byte == m->sendFrame.addr) ?
					e_mbgRxState_funcCode : e_mbgRxState_waitForTout;
			break;
		}

		case e_mbgRxState_funcCode:
		{
			m->mbg.rxFrame.code = byte;

			// check match with sent func code
			m->mbg.rxState = (byte == m->sendFrame.code) ?
					e_mbgRxState_data : e_mbgRxState_waitForTout;
			break;
		}

		case e_mbgRxState_data:
		{
			switch (m->mbg.rxFrame.code)
			{
				case e_mbFuncCode_WriteSingleCoil:
				case e_mbFuncCode_WriteSingleRegister:
				{
					m->mbg.rxFrame.data[m->mbg.rxFrame.dataLen++] = byte;

					if (m->mbg.rxFrame.dataLen >= 4) // gather 4 bytes
						m->mbg.rxState = e_mbgRxState_crcHi;
					break;
				}

				case e_mbFuncCode_ReadHoldingRegisters:
				{
					m->mbg.rxFrame.data[m->mbg.rxFrame.dataLen++] = byte; // byte count

					// if no bytes to read move to crc
					m->mbg.rxState = byte ? e_mbgRxState_data2 : e_mbgRxState_crcHi;
					break;
				}

				default:
				{
					// unhandled code, wait for timeout
					m->mbg.rxState = e_mbgRxState_waitForTout;
				}
			}
			break;
		}

		case e_mbgRxState_data2:
		{
			switch (m->mbg.rxFrame.code)
			{
				case e_mbFuncCode_ReadHoldingRegisters:
				{
					m->mbg.rxFrame.data[m->mbg.rxFrame.dataLen++] = byte;

					if (m->mbg.rxFrame.dataLen >= (m->mbg.rxFrame.data[0] + 1))
						m->mbg.rxState = e_mbgRxState_crcHi;
					break;
				}

				default:
				{
					// unhandled code, wait for timeout
					m->mbg.rxState = e_mbgRxState_waitForTout;
				}
			}
			break;
		}

		case e_mbgRxState_crcHi: // CRC HI
		{
			m->mbg.rxFrame.crc = (uint16_t)byte << 8;
			m->mbg.rxState = e_mbgRxState_crcLo;
			break;
		}

		case e_mbgRxState_crcLo: // CRC LO
		{
			m->mbg.rxFrame.crc |= byte;

			// handle the frame
			mbm_rxFrameHandle(m);
			break;
		}

		case e_mbgRxState_waitForTout:
		default:
		{
			// Do nothing, wait for timeout to reset the receiver
		}
	}
}

/*
 * @brief	Handles the received frame.
 */
void mbm_rxFrameHandle(mbmUart_t* const mbmu)
{
	assert_param(mbmu);
	mbgFrame_t* const mf = &mbmu->mbg.rxFrame;
	uint16_t calcCrc;

	// disable receiving
	mbg_DisableReceiver(&mbmu->mbg);

	// turn off response timeout timer
	mbm_RespTimeoutStop(mbmu);

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

			default:
			{
				// should not happen
			}
		}
	}

	// CRC mismatch
	else
		mbm_RespCrcMatchError(mf->crc, calcCrc);
}

/*
 * @brief	ModBus MASTER rx timeout override.
 * @param	tim: pointer to the modbus timeout timer strict
 */
void mbm_uartRxTimeoutRoutine(TIM_HandleTypeDef* tim)
{
	assert_param(tim);

	// check if handle is known
	mbmUart_t* mbm = mbm_GetModuleFromTimer(tim);

	// allow user to handle the timeout
	mbm_RespRxTimeoutError(mbm->mbg.rxState, &mbm->mbg.rxFrame);

	if (mbm)
		mbg_RxTimeout(&mbm->mbg);
}

/*
 * @brief	ModBus MASTER strong override function.
 * 			Use it to parse RESPONSES from the slave device.
 * 			Return if \ref uHandle does not match configured periph.
 * @param	uHandle: pointer to the uart modbus slave struct.
 */
void mbm_uartRxRoutine(UART_HandleTypeDef* uHandle)
{
	assert_param(uHandle);

	// check if handle is known
	mbmUart_t* mbm = mbm_GetModuleFromUart(uHandle);

	if (mbm)
		mbg_ByteReceived(&mbm->mbg);
}

/*
 * @brief	ModBus MASTER strong override function.
 * 			Use it to turn on the received after succesfull request is sent.
 * 			Return if \ref uHandle does not match configured periph.
 * @param	uHandle: pointer to the uart modbus slave struct.
 */
void mbm_uartTxRoutine(UART_HandleTypeDef* uHandle)
{
	assert_param(uHandle);

	// check if handle is known
	mbmUart_t* mbm = mbm_GetModuleFromUart(uHandle);

	if (mbm)
	{
		// check if request was not a broadcast
		if (!mbm->sendFrame.addr)
			return;

		// reset modbus rx state
		mbg_RxTimeout(&mbm->mbg);

		// start global timeout timer
		mbm_RespTimeoutRestart(mbm);
	}
}

/*
 * @brief	(Re)Starts the response timeout timer.
 * @param	mbmu: Modbus master module pointer
 */
HAL_StatusTypeDef mbm_RespTimeoutRestart(mbmUart_t* mbmu)
{
	assert_param(mbmu);

	return (HAL_StatusTypeDef)osMessagePut(
			mbmu->toutQ.msgQId, mbmu->toutQ.timeout_ms, osWaitForever);
}

/*
 * @brief	Stop the response timeout timer.
 * @param	mbmu: Modbus master module pointer
 */
HAL_StatusTypeDef mbm_RespTimeoutStop(mbmUart_t* mbmu)
{
	assert_param(mbmu);

	return (HAL_StatusTypeDef)osMessagePut(mbmu->toutQ.msgQId, 0, osWaitForever);
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












