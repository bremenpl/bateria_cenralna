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
static modbusFrame_t mbs_rxFrames[MBS_RCV_QUEUE_SIZE];
static uint32_t mbs_rxFramesIndex;

static UART_HandleTypeDef* mbs_uartHandle;
static uint8_t mbs_address;
static mbsRxState_t mbs_rxState;

/* Public variables ----------------------------------------------------------*/

/* Fuction prototypes --------------------------------------------------------*/
void mbs_task_rxDequeue(void const* argument);
HAL_StatusTypeDef mbs_ExamineFuncCode(uint8_t fCode, uint8_t* data_p, uint32_t* len);

/* Function declarations -----------------------------------------------------*/

/*
 * @brief	Initializes the MODBUS Slave controller.
 * @param	uartHandle: pointer to a uart HAL handle struct.
 * @param	address: slave device address (1 - 247)
 * @return	HAL_OK if initialization was succesfull.
 */
HAL_StatusTypeDef mbs_Init(UART_HandleTypeDef* uartHandle, uint8_t address)
{
	// check pointer
	assert_param(uartHandle);
	if (!uartHandle)
		return HAL_ERROR;

	// validate slave address
	if ((address < MBS_MIN_ADDR) || (address > MBS_MAX_ADDR))
		return HAL_ERROR;

	HAL_StatusTypeDef retVal = HAL_OK;

	// assign handle, address, initialize uart
	mbs_uartHandle = uartHandle;
	mbs_address = address;
	retVal += mbg_UartInit(mbs_uartHandle);

	// create master requests reception queue. In theory one item queue should be enough.
	// Add some buffer just to be sure.
	osMessageQDef_t msgDef_temp;
	msgDef_temp.item_sz = sizeof(modbusFrame_t*);
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

	assert_param(!retVal);
	return HAL_OK;
}

/*
 * @brief	Call this method in function code examination routine.
 * @param	fCode: Input parameter, received function code to examine.
 * @param	data_p: Output parameter,
 */
HAL_StatusTypeDef mbs_ExamineFuncCode(uint8_t fCode, uint8_t* data_p, uint32_t* len)
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
			break;
		}

		case e_mbFuncCode_WriteMultipleRegisters:
		{
			break;
		}

		default: retVal =  HAL_ERROR; // unknown function code
	}

	return retVal;
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
	if (uHandle != mbs_uartHandle)
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
			if (!mbs_ExamineFuncCode(mbs_rxFrames[mbs_rxFramesIndex].code, data_p, &len))
				mbs_rxState = e_mbsRxState_data;
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
	if (uHandle != mbs_uartHandle)
		return;

	// We are in t3.5 rx timeout routine,
	// meaning modbus frame has to be gathered from the beginning.

	// Enable receiver but without rx timeout, wait forever for the 1st address byte
	mbg_EnableReceiver(mbs_uartHandle, &mbs_rxFrames[mbs_rxFramesIndex].addr, 1, 0);

	// set rxState to the beginning
	mbs_rxState = e_mbsRxState_addr;

	// debug
	HAL_GPIO_TogglePin(GPIOA, 1 << 5);
}

/*
 * @brief	Dequeue task for handling ready MODBUS requests tied together in
 * 			the uart RX interrupt routine.
 */
void mbs_task_rxDequeue(void const* argument)
{
	// Initially enable receiver for the 1st address byte, no rx timeout
	mbg_EnableReceiver(mbs_uartHandle, &mbs_rxFrames[mbs_rxFramesIndex].addr, 1, 0);

	while (1)
	{
		//HAL_GPIO_TogglePin(GPIOA, 1 << 5);
		osDelay(100);
	}
}







