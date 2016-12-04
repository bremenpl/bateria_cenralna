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

static osMessageQId mbs_msgQId_rX;
static osThreadId mbs_sysId_rX;
static mbgFrame_t mbs_rxFrames[MBS_RCV_QUEUE_SIZE];
static uint32_t mbs_rxFramesIndex;

static mbgUart_t mbs_uart;
static uint8_t mbs_address;
static mbgRxState_t mbs_rxState;

/* Public variables ----------------------------------------------------------*/

/* Fuction prototypes --------------------------------------------------------*/
void mbs_task_rxDequeue(void const* argument);
HAL_StatusTypeDef mbs_ExamineFuncCode(const uint8_t fCode, uint8_t** data_p, uint32_t* len);

/* Function declarations -----------------------------------------------------*/

/*
 * @brief	Initializes the MODBUS Slave controller.
 * @param	uartHandle: pointer to a uart HAL handle struct.
 * @param	address: slave device address (1 - 247)
 * @return	HAL_OK if initialization was succesfull.
 */
HAL_StatusTypeDef mbs_Init(UART_HandleTypeDef* uartHandle, uint8_t address)
{
	if (!uartHandle)
		return HAL_ERROR;

	// validate slave address
	if ((address < MBS_MIN_ADDR) || (address > MBS_MAX_ADDR))
		return HAL_ERROR;

	HAL_StatusTypeDef retVal = HAL_OK;

	// assign handle, address, initialize uart
	mbs_uart.handle = uartHandle;
	mbs_address = address;
	retVal += mbg_UartInit(&mbs_uart);

	// create master requests reception queue. In theory one item queue should be enough.
	// Add some buffer just to be sure.
	osMessageQDef_t msgDef_temp;
	msgDef_temp.item_sz = sizeof(mbgFrame_t*);
	msgDef_temp.queue_sz = MBS_RCV_QUEUE_SIZE;

	// create the queue
	mbs_msgQId_rX = osMessageCreate(&msgDef_temp, NULL);
	if (!mbs_msgQId_rX)
		retVal++;

	// create thread for dequeuing
	osThreadDef(mbsRxTask, mbs_task_rxDequeue, osPriorityAboveNormal, 0, 256);
	mbs_sysId_rX = osThreadCreate(osThread(mbsRxTask), NULL);
	if (!mbs_sysId_rX)
		retVal++;

	// defaulr rx state
	mbs_rxState = e_mbsRxState_addr;

	return retVal;
}

/*
 * @brief	Call this method in function code examination routine.
 * @param	fCode: Input parameter, received function code to examine.
 * @param	data_p: data pointer under which rx data will be saved,
 * @param	len: length of received data.
 */
HAL_StatusTypeDef mbs_ExamineFuncCode(const uint8_t fCode, uint8_t** data_p, uint32_t* len)
{
	assert_param(fCode);
	assert_param(data_p);
	assert_param(len);
	HAL_StatusTypeDef retVal = HAL_OK;

	// decide what to do next depending on the function code
	switch (fCode)
	{
		case e_mbFuncCode_ReadHoldingRegisters:
		{
			// For this request one has to wait for 4 bytes:
			// Starting address (2), quantity of registers (2)
			*data_p = mbs_rxFrames[mbs_rxFramesIndex].data;
			*len = 4;

			// save length for generic purposes, like crc calculation
			mbs_rxFrames[mbs_rxFramesIndex].dataLen = *len;
			break;
		}

		/*case e_mbFuncCode_WriteMultipleRegisters:
		{
			break;
		}*/

		default:
		{
			// same principle as for 1st byte
			*data_p = &mbs_rxFrames[mbs_rxFramesIndex].code;
			*len = MBG_MAX_DATA_LEN + 3;
			retVal =  HAL_ERROR; // unknown function code
		}
	}

	return retVal;
}

/*
 * @brief	Send an exception code frame to the master in response to a request.
 * @param	mf: pointer to a received modbus frame, that will now be used as data
 * 			container for creating a response frame. This is done like this due to
 * 			memory and speed optimalisation reasons.
 * 			The protocol is half duplex so its safe.
 * @param	exCode: Exception code of type \ref mbsExCode_t.
 * @return
 */
HAL_StatusTypeDef mbs_SendErrorResponse(mbgFrame_t* mf, mbgExCode_t exCode)
{
	if (!mbs_uart.handle || !mf)
		return HAL_ERROR;

	// modify function code (becomes error code)
	mf->code |= MBS_EXCODE_ADDR_OR;

	// save exception code in data field
	mf->data[0] = (uint8_t)exCode;

	// update data length
	mf->dataLen = 1;

	// send the data and return
	return mbg_SendFrame(&mbs_uart, mf);
}

/*
 * @brief	ModBus SLAVE strong override function.
 * 			Use it to parse REQUESTS from the master device.
 * 			Return if \ref uHandle does not match configured periph.
 * @param	uHandle: pointer to the uart modbus slave struct.
 */
void mbs_uartRxRoutine(UART_HandleTypeDef* uHandle)
{
	// check either pointer is proper
	if (uHandle != mbs_uart.handle)
		return;

	// define operational vars, set default values
	uint8_t* data_p = 0;
	uint32_t len = 0;
	bool rxTimeoutSet = true;

	// enter state machine and comlete the modbus request frame
	switch (mbs_rxState)
	{
		case e_mbsRxState_addr: // Obtained slave address, check either it matches
		{
			if (mbs_rxFrames[mbs_rxFramesIndex].addr == mbs_address)
			{
				// request adressed to me, get the function code
				data_p = &mbs_rxFrames[mbs_rxFramesIndex].code;
				len = 1;
				mbs_rxState = e_mbsRxState_funcCode;
			}
			else // no match, wait for rx timeout
			{
				/*
				 * Gather dummy bytes from code member, just to keep rx DMA on.
				 * In theory this is safe, at max possible length data,
				 * function code and crc members should be filled.
				 */
				data_p = &mbs_rxFrames[mbs_rxFramesIndex].code;
				len = MBG_MAX_DATA_LEN + 3; // 3 for fcode + crc at max length
			}
			break;
		}

		case e_mbsRxState_funcCode: // obtained function code
		{
			// if the function is known, go further. If not, start from the begging
			// after timeout
			if (!mbs_ExamineFuncCode(mbs_rxFrames[mbs_rxFramesIndex].code, &data_p, &len))
				mbs_rxState = e_mbsRxState_data;
			break;
		}

		case e_mbsRxState_data: // received the data
		{
			// now its tie to receive 16 bit CRC
			data_p = (uint8_t*)&mbs_rxFrames[mbs_rxFramesIndex].crc;
			len = 2;
			mbs_rxState = e_mbsRxState_crc;
			break;
		}

		case e_mbsRxState_crc:
		{
			// Add a completed frame to the queue, crc will be calculated there
			// then wait for the rx timeout to restart the receiver

			if (osOK == osMessagePut(mbs_msgQId_rX,
					(uint32_t)&mbs_rxFrames[mbs_rxFramesIndex], 0))
			{
				if (++mbs_rxFramesIndex >= MBS_RCV_QUEUE_SIZE)
					mbs_rxFramesIndex = 0;
			}
			break;
		}

		default: // unknown state, wait for addr byte without timeut
		{
			data_p = &mbs_rxFrames[mbs_rxFramesIndex].addr;
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
 * 			Use it to parse receiver timeouts.
 * 			Return if \ref uHandle does not match configured periph.
 * @param	uHandle: pointer to the uart modbus slave struct.
 */
void mbs_uartRxTimeoutRoutine(UART_HandleTypeDef* uHandle)
{
	// check either pointer is proper
	if (uHandle != mbs_uart.handle)
		return;

	// We are in t3.5 rx timeout routine,
	// meaning modbus frame has to be gathered from the beginning.

	// Enable receiver but without rx timeout, wait forever for the 1st address byte
	mbg_EnableReceiver(mbs_uart.handle, &mbs_rxFrames[mbs_rxFramesIndex].addr, 1, 0);

	// set rxState to the beginning
	mbs_rxState = e_mbsRxState_addr;
}

/*
 * @brief	Dequeue task for handling ready MODBUS requests tied together in
 * 			the uart RX interrupt routine.
 */
void mbs_task_rxDequeue(void const* argument)
{
	// Initially enable receiver for the 1st address byte, no rx timeout
	mbg_EnableReceiver(mbs_uart.handle, &mbs_rxFrames[mbs_rxFramesIndex].addr, 1, 0);

	osEvent retEvent;
	mbgExCode_t exCode;
	mbgFrame_t* mf;

	while (1)
	{
		/*
		 * 	Peek is used instead of get, in order to keep the message in the queue
		 *	for the time of response creation and sending. the message gas to be
		 *	dequeued eventually after response is sent.
		 */
		retEvent = osMessagePeek(mbs_msgQId_rX, osWaitForever);

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
						exCode = mbs_CheckReadHoldingRegistersRequest(mf);
						break;
					}

					// for unknown function ILLEGAL FUNCTION error answer code
					// has to be sent
					default: exCode = e_mbsExCode_illegalFunction;
				}

				// if exception code set, send error response
				if (exCode)
					mbs_SendErrorResponse(mf, exCode);

				// otherwise standard response.
				// Override func has to fill mf with data.
				else
					mbg_SendFrame(&mbs_uart, mf);
			}

			// finally remove the item from the queue. No need to check if there is
			// any message, because this thread is the only dequeuer.
			osMessageGet(mbs_msgQId_rX, osWaitForever);
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
__attribute__((weak)) mbgExCode_t mbs_CheckReadHoldingRegistersRequest(mbgFrame_t* mf)
{
	return e_mbsExCode_illegalDataAddr;
}







