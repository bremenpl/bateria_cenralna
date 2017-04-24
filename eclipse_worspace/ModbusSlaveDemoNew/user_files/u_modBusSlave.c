/*
 * u_modBusSlave.c
 *
 *  Created on: 01.03.2017
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

HAL_StatusTypeDef mbs_SendErrorResponse(mbgUart_t* uart, mbgFrame_t* mf, mbgExCode_t exCode);
mbsUart_t* mbs_GetModuleFromUart(UART_HandleTypeDef* uart);
mbsUart_t* mbs_GetModuleFromTimer(TIM_HandleTypeDef* timer);
mbsUart_t* mbs_GetModuleFromMbg(mbgUart_t* mbg);
mbsUart_t* mbs_GetModuleFromPlc(plcm_t* plc);

void mbs_digForFrames(mbsUart_t* m, const uint8_t byte);
void mbs_rxFrameHandle(mbsUart_t* const mbsu);
void mbs_taskRxDequeue(void const* argument);

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
	osThreadDef(mbgRxTask, mbs_taskRxDequeue, osPriorityAboveNormal, noOfModules, 256);

	// do everything for each module
	for (size_t i = 0; i < noOfModules; i++)
	{
		// Convenience
		m = mbsu + i;

		// check mbsu pointers
		if (!m)
			return HAL_ERROR;

		// validate slave address
		if ((m->slaveAddr < MBS_MIN_ADDR) || (m->slaveAddr > MBS_MAX_ADDR))
			return HAL_ERROR;

		// assign parameter variables
		mbs_u[i] = m;

		// init the generic uart module
		retVal += mbg_UartInit(&m->mbg);

		// define queue
		msgDef_temp.item_sz = sizeof(uint32_t);
		msgDef_temp.queue_sz = MBG_RECV_QUEUE_LEN; // can accumulate up to 16 bytes

		// create the queue
		m->mbg.rxQ.msgQId = osMessageCreate(&msgDef_temp, NULL);
		if (!m->mbg.rxQ.msgQId)
			retVal++;

		// create thread for dequeuing
		m->mbg.rxQ.sysId = osThreadCreate(osThread(mbgRxTask), &m->mbg);
		if (!m->mbg.rxQ.sysId)
			retVal++;

		// defaulr rx state
		m->mbg.rxState = e_mbgRxState_addr;
	}

	assert_param(!retVal);
	return retVal;
}

/*
 * @brief	If \ref uart is found in one of the slave structs, the slave struct
 * 			in which it was found is returned. Otherwised zero is returned.
 * @param	uart: handle
 * @return	modbus slave uart module pointer or zero.
 */
mbsUart_t* mbs_GetModuleFromUart(UART_HandleTypeDef* uart)
{
	assert_param(uart);

	for (size_t i = 0; i < mbs_slavesNr; i++)
	{
		if (uart == mbs_u[i]->mbg.handle)
			return mbs_u[i];
	}

	return 0;
}

/*
 * @brief	If \ref timer is found in one of the slave structs, the slave struct
 * 			in which it was found is returned. Otherwised zero is returned.
 * @param	timer: handle
 * @return	modbus slave timer module pointer or zero.
 */
mbsUart_t* mbs_GetModuleFromTimer(TIM_HandleTypeDef* timer)
{
	assert_param(timer);

	for (size_t i = 0; i < mbs_slavesNr; i++)
	{
		if (timer == mbs_u[i]->mbg.rxQ.toutTim)
			return mbs_u[i];
	}

	return 0;
}

/*
 * @brief	If \ref mbg is found in one of the slave structs, the slave struct
 * 			in which it was found is returned. Otherwised zero is returned.
 * @param	timer: handle
 * @return	modbus generic module pointer or zero.
 */
mbsUart_t* mbs_GetModuleFromMbg(mbgUart_t* mbg)
{
	assert_param(mbg);

	for (size_t i = 0; i < mbs_slavesNr; i++)
	{
		if (mbg == &mbs_u[i]->mbg)
			return mbs_u[i];
	}

	return 0;
}

/*
 * @brief	If \ref plc is found in one of the slave structs, the slave struct
 * 			in which it was found is returned. Otherwise zero is returned.
 * @param	plc: handle
 * @return	modbus master plc module pointer or zero.
 */
mbsUart_t* mbs_GetModuleFromPlc(plcm_t* plc)
{
	assert_param(plc);

	for (size_t i = 0; i < mbs_slavesNr; i++)
	{
		if (plc == &mbs_u[i]->mbg.plcm)
			return mbs_u[i];
	}

	return 0;
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
 * @brief	Rx dequeue task. Raw bytes received by slave are examined here.
 * @param	argument: Pass mbg pointer struct here.
 */
void mbs_taskRxDequeue(void const* argument)
{
	assert_param(argument);

	mbsUart_t* m = mbs_GetModuleFromMbg((mbgUart_t*)argument);
	osEvent retEvent;

	while (1)
	{
		retEvent = osMessageGet(m->mbg.rxQ.msgQId, osWaitForever);

		if (retEvent.status == osEventMessage)
			mbs_digForFrames(m, (uint8_t)retEvent.value.v);
	}
}

/*
 * @brief	Run this function each time new byte arrives. It examines each byte (while
 * 			remembering the state) and takes actions if the frame is complete. this
 * 			function should not be run inside an interrupt.
 */
void mbs_digForFrames(mbsUart_t* m, const uint8_t byte)
{
	assert_param(m);
	assert_param(!mbg_inHandlerMode());

	// enter state machine and comlete the modbus request frame
	switch (m->mbg.rxState)
	{
		case e_mbgRxState_addr: // Obtained slave address, check either it matches
		{
			m->mbg.rxFrame.addr = byte;
			m->mbg.rxFrame.dataLen = 0;

			// request adressed to me, get the function code,
			// otherwise wait for timeout
			m->mbg.rxState = ((byte == m->slaveAddr) || !byte) ?
					e_mbgRxState_funcCode : e_mbgRxState_waitForTout;
			break;
		}

		case e_mbgRxState_funcCode: // obtained function code
		{
			m->mbg.rxFrame.code = byte;
			m->mbg.rxState = e_mbgRxState_data;
			break;
		}

		case e_mbgRxState_data: // received the data (length/ registers)
		{
			switch (m->mbg.rxFrame.code)
			{
				case e_mbFuncCode_WriteSingleCoil:
				case e_mbFuncCode_WriteSingleRegister:
				case e_mbFuncCode_ReadCoils:
				case e_mbFuncCode_ReadHoldingRegisters:
				{
					m->mbg.rxFrame.data[m->mbg.rxFrame.dataLen++] = byte;

					if (m->mbg.rxFrame.dataLen >= 4) // gather 4 bytes
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
			//m->mbg.rxState = e_mbgRxState_waitForToutRdyFrame;
			mbs_rxFrameHandle(m);
			break;
		}

		case e_mbgRxState_waitForTout:
		case e_mbgRxState_waitForToutRdyFrame:
		default:
		{
			// Do nothing, wait for timeout to reset the receiver
		}
	}
}

/*
 * @brief	Handles the received frame.
 * @brief	mf: received frame
 */
void mbs_rxFrameHandle(mbsUart_t* const mbsu)
{
	assert_param(mbsu);
	mbgExCode_t exCode;
	mbgFrame_t* const mf = &mbsu->mbg.rxFrame;

	// disable receiving
	mbg_DisableReceiver(&mbsu->mbg);

	// delay the response as the T3.5 timeout would
	osDelay(mbsu->mbg.mbTimeout_ms);

	// validate crc
	if (!mbg_CheckCrc(mf, 0))
	{
		// print the request
		mf->msgType = e_mbgMesgType_Request;
		mbg_uartPrintFrame(mf);

		// now set to response
		mf->msgType = e_mbgMesgType_Response;

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
				exCode = mbs_CheckWriteSingleCoil(mbsu, mf);
				break;
			}

			// for unknown function ILLEGAL FUNCTION error answer code has to be sent
			default: exCode = e_mbsExCode_illegalFunction;
		}

		// send response only if not broadcast and excode is not a delegate
		if (mf->addr && (exCode != e_mbsExCode_delegate))
		{
			// if exception code set, send error response
			if (exCode)
				mbs_SendErrorResponse(&mbsu->mbg, mf, exCode);

			// otherwise standard response. Override func has to fill mf with data.
			else
				mbg_SendFrame(&mbsu->mbg, mf);
		}
		else
			mbg_RxTimeout(&mbsu->mbg); // reset receiver
	}
	else // bad crc, wait for timeout
		mbg_RxTimeout(&mbsu->mbg); // reset receiver
}

/*
 * @brief	ModBus SLAVE rx timeout override.
 * @param	tim: pointer to the modbus timeout timer strict
 */
void mbs_uartRxTimeoutRoutine(TIM_HandleTypeDef* tim)
{
	assert_param(tim);

	// check if handle is known
	mbsUart_t* mbs = mbs_GetModuleFromTimer(tim);

	if (mbs)
		mbg_RxTimeout(&mbs->mbg);
}

/*
 * @brief	ModBus SLAVE strong override function.
 * 			Use it to parse REQUESTS from the master device.
 * 			Return if \ref uHandle does not match configured periph.
 * @param	uHandle: pointer to the uart modbus slave struct.
 */
void mbs_uartRxRoutine(UART_HandleTypeDef* uHandle)
{
	assert_param(uHandle);

	// check if handle is known
	mbsUart_t* mbs = mbs_GetModuleFromUart(uHandle);

	if (mbs)
		mbg_ByteReceived(&mbs->mbg);
}

/*
 * @brief	ModBus SLAVE strong override function.
 * 			Use it to take actions after response has been sent.
 * 			Return if \ref uHandle does not match configured periph.
 * @param	uHandle: pointer to the uart modbus slave struct.
 */
void mbs_uartTxRoutine(UART_HandleTypeDef* uHandle)
{
	assert_param(uHandle);

	// check if handle is known
	mbsUart_t* mbs = mbs_GetModuleFromUart(uHandle);

	if (mbs)
		mbg_RxTimeout(&mbs->mbg);
}

void mbs_plcRxRoutine(plcm_t* plcHandle)
{
	assert_param(plcHandle);

	// check if handle is known
	mbsUart_t* mbs = mbs_GetModuleFromPlc(plcHandle);

	if (mbs)
	{
		// move data from plc layer to mbg layer
		mbs->mbg.rxQ.rxByte = *mbs->mbg.plcm.trans.data;
		mbg_ByteReceived(&mbs->mbg);
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
__attribute__((weak)) mbgExCode_t mbs_CheckWriteSingleCoil(const mbsUart_t* const mbs,
		mbgFrame_t* mf)
{
	return e_mbsExCode_illegalDataAddr;
}








