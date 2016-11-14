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

static UART_HandleTypeDef* mbs_uartHandle;

/* Public variables ----------------------------------------------------------*/

/* Fuction prototypes --------------------------------------------------------*/
void msb_task_rxDequeue(void const* argument);

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

	// assign handle
	mbs_uartHandle = uartHandle;

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
	osThreadDef(mbsRxTask, msb_task_rxDequeue, osPriorityAboveNormal, 0, 256);
	mbs_sysId_rX = osThreadCreate(osThread(mbsRxTask), NULL);
	if (!mbs_sysId_rX)
		retVal++;

	retVal += HAL_UART_Receive_DMA(mbs_uartHandle, mbs_rXFrames[0].data, 1);

	assert_param(!retVal);
	return HAL_OK;
}

void msb_task_rxDequeue(void const* argument)
{
	uint8_t times = 0;

	while (1)
	{
		HAL_GPIO_TogglePin(GPIOA, 1 << 5);
		osDelay(++times);
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart != mbs_uartHandle)
		return;


}



