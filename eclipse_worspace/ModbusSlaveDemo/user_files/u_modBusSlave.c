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
static modbusFrame_t mbs_rXFrames[MBS_RCV_QUEUE_SIZE];
static uint32_t mbs_rxFramesIndex;
static uint8_t mbs_rawRxDataStream[sizeof(modbusFrame_t)];

static UART_HandleTypeDef* mbs_uartHandle;

/* Public variables ----------------------------------------------------------*/

/* Fuction prototypes --------------------------------------------------------*/
void mbs_task_rxDequeue(void const* argument);
HAL_StatusTypeDef mbs_EnableReceiver(const uint16_t len);

/* Function declarations -----------------------------------------------------*/

/*
 * @brief	Initializes the MODBUS Slave controller.
 * @param	uartHandle: pointer to a uart HAL handle struct.
 * @return	HAL_OK if initialization was succesfull.
 */
HAL_StatusTypeDef mbs_Init(UART_HandleTypeDef* uartHandle)
{
	assert_param(uartHandle);
	if (!uartHandle)
		return HAL_ERROR;

	HAL_StatusTypeDef retVal = HAL_OK;

	// assign handle, initialize uart
	mbs_uartHandle = uartHandle;
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

	assert_param(!retVal);
	return HAL_OK;
}

/*
 * @brief	ModBus SLAVE strong override function.
 * 			Use it to parse REQUESTS from the master device.
 * @param	uHandle: pointer to the uart modbus slave struct.
 */
void mbs_uartRxRoutine(UART_HandleTypeDef* uHandle)
{
	// check either pointer is proper
	if (uHandle != mbs_uartHandle)
		return;


}

/*
 * @brief	Dequeue task for handling ready MODBUS requests tied together in
 * 			the uart RX interrupt routine.
 */
void mbs_task_rxDequeue(void const* argument)
{
	// Initially enable receiver for the 1st address byte
	mbs_EnableReceiver(1);

	uint8_t times = 0;
	while (1)
	{
		HAL_GPIO_TogglePin(GPIOA, 1 << 5);
		osDelay(++times);
	}
}

/*
 * @brief	Enable half duplex uart DMA receiver for \ref len number of bytes.
 * @param	len: amount of bytes to receive.
 * @return	HAL_OK if inits succesfull.
 */
HAL_StatusTypeDef mbs_EnableReceiver(const uint16_t len)
{
	if (!len)
		return HAL_OK; // nothing sent

	assert_param(mbs_uartHandle);
	HAL_StatusTypeDef retVal = HAL_OK;

	retVal += HAL_HalfDuplex_EnableReceiver(mbs_uartHandle); // should toggle DE?
	retVal += HAL_UART_Receive_DMA(mbs_uartHandle, mbs_rawRxDataStream, len);

	return retVal;
}





