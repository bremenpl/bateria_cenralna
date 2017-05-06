/*
 * u_plcMaster.c
 *
 *  Created on: 07.04.2017
 *      Author: Lukasz
 */

/* Includes ------------------------------------------------------------------*/
#include "u_plc.h"

/* Defines and macros --------------------------------------------------------*/

/* Enums and structs ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static plcm_t* plcm[PLC_MAX_MODULES];	/*!< global plcm module */
static uint32_t plcIndex = 0;

/* Public variables ----------------------------------------------------------*/

/* Fuction prototypes --------------------------------------------------------*/

HAL_StatusTypeDef plcm_SetTimerParams(const timParams_t* const tp, TIM_HandleTypeDef* timer);
HAL_StatusTypeDef plcm_SyncTimerRestart(const uint32_t id);
HAL_StatusTypeDef plcm_SyncTimerStop(const uint32_t id);
HAL_StatusTypeDef plcm_BitsTimerRestart(const uint32_t id);
HAL_StatusTypeDef plcm_BitsTimerStop(const uint32_t id);
HAL_StatusTypeDef plcm_ZcTimerRestart(const uint32_t id);

HAL_StatusTypeDef plcm_GetIdFromZcPin(uint16_t GPIO_Pin, uint32_t* id);
HAL_StatusTypeDef plcm_GetIdFromInPin(uint16_t GPIO_Pin, uint32_t* id);
HAL_StatusTypeDef plcm_GetIdFromSyncTim(TIM_HandleTypeDef* timer, uint32_t* id);
HAL_StatusTypeDef plcm_GetIdFromBitsTim(TIM_HandleTypeDef* timer, uint32_t* id);
HAL_StatusTypeDef plcm_GetIdFromZcTim(TIM_HandleTypeDef* timer, uint32_t* id);
HAL_StatusTypeDef plcm_SendSyncBit(const uint32_t id);
void plcm_SendDataBit(const uint32_t id);
HAL_StatusTypeDef plcm_SetIdleState(const uint32_t id);
HAL_StatusTypeDef plcm_FinishSendChunk(const uint32_t id);
void plcm_ReadDataBit(const uint32_t id);


/* Function declarations -----------------------------------------------------*/

/*
 * @brief	Initializes the PLC module.
 * @param	p: pointer to a preconfigured plc struct.
 * @param	id: pointer under which plc module id is saved.
 * @param	HAL_OK when success, value saved under id is valid.
 */
HAL_StatusTypeDef plcm_Init(plcm_t* p, uint32_t* id)
{
	assert_param(id);
	assert_param(p);
	assert_param(p->timHandleSync);
	assert_param(p->timHandleBits);
	assert_param(p->plcIngpio);
	assert_param(p->plcInpin);
	assert_param(p->plcOutgpio);
	assert_param(p->plcOutpin);
	HAL_StatusTypeDef retVal = HAL_OK;

	// check either sync and bits timers are not the same
	if (p->timHandleSync == p->timHandleBits)
		return HAL_ERROR;

	// if some modules are declared already, check if provided parameters dont overlay them
	if (plcIndex)
	{
		assert_param(p->zCgpio);
		assert_param(p->zCpin);

		if (plcIndex >= (PLC_MAX_MODULES - 1))
			return HAL_ERROR; // cannot add more

		for (uint32_t i = 0; i < plcIndex; i++)
		{
			// check timers
			if (plcm[i]->timHandleSync == p->timHandleSync)
				return HAL_ERROR;

			if (plcm[i]->timHandleBits == p->timHandleBits)
				return HAL_ERROR;

			// compare gpios only when devices are masters
			if (e_plcDevType_Master == p->devType)
			{
				if (e_plcDevType_Master == plcm[i]->devType)
				{
					// IN GPIO
					if ((p->plcIngpio == plcm[i]->plcIngpio) &&
							(p->plcInpin == plcm[i]->plcInpin))
						return HAL_ERROR;

					// OUT GPIO
					if ((p->plcOutgpio == plcm[i]->plcOutgpio) &&
							(p->plcOutpin == plcm[i]->plcOutpin))
						return HAL_ERROR;

					// ZC GPIO, check only input number (using external interrupts)
					if (p->zCpin == plcm[i]->zCpin)
						return HAL_ERROR;
				}
			}
		}
	}

	// initialize the bits timer for 1 ms timeout (transmitting)
	retVal += plcm_SetTimerFreq(1000.0f, p->timHandleBits);

	// save values
	if (!retVal)
	{
		p->timParamsBitsSend.period = p->timHandleBits->Init.Period;
		p->timParamsBitsSend.prescaler = p->timHandleBits->Init.Prescaler;
	}
	else
		return HAL_ERROR;

	// initialize the bit timer for 1.5 ms timeout (reception)
	retVal += plcm_SetTimerFreq(666.0f, p->timHandleBits);

	// save values
	if (!retVal)
	{
		p->timParamsBitsRecv.period = p->timHandleBits->Init.Period;
		p->timParamsBitsRecv.prescaler = p->timHandleBits->Init.Prescaler;
	}
	else
		return HAL_ERROR;

	// set timers params to "send" ones if the module is master
	if (e_plcDevType_Master == p->devType)
	{
		assert_param(p->zCgpio);
		assert_param(p->zCpin);
		retVal += plcm_SetTimerParams(&p->timParamsBitsSend, p->timHandleBits);
	}

	// initialize sync timer to 10 ms (for sending)
	retVal += plcm_SetTimerFreq(100.0f, p->timHandleSync);

	// save values
	if (!retVal)
	{
		p->timParamsSyncSend.period = p->timHandleSync->Init.Period;
		p->timParamsSyncSend.prescaler = p->timHandleSync->Init.Prescaler;
	}
	else
		return HAL_ERROR;

	// Initialize ZC timer for AC/DC checking
	if (e_plcDevType_Master == p->devType)
	{
		assert_param(p->timHandleZc);
		retVal += plcm_SetTimerFreq(100.0f, p->timHandleZc);
	}

	// assign the module
	if (!retVal)
	{
		plcm[plcIndex] = p;
		*id = plcIndex++;
		p->state = e_plcState_Idle;

		if (e_plcDevType_Master == p->devType)
			p->supply = e_plcSupplyType_Unspecified;
		else
			p->supply = e_plcSupplyType_Dc;  // slave always on DC
	}

	retVal += plcm_SetIdleState(*id);

	// If master, start to check either working on AC or DC
	if (e_plcDevType_Master == p->devType)
		retVal += plcm_ZcTimerRestart(*id);

	return retVal;
}

/*
 * @brief	Sets the provided timer frequency
 * @param	freq: frequency to be set.
 * @param	timer: timer struct
 * @return	HAL_OK if freq set
 */
HAL_StatusTypeDef plcm_SetTimerFreq(const float freq, TIM_HandleTypeDef* timer)
{
	assert_param(timer);
	assert_param(freq != 0);
	HAL_StatusTypeDef retVal = HAL_OK;

	// now find proper prescaller and period for \ref tim
	uint16_t period[0xFF];
	uint32_t prescaler = 0;
	float F_mcu = HAL_RCC_GetHCLKFreq();

	for (prescaler = 0; prescaler < 0xFF; prescaler++)
		period[prescaler] = (F_mcu / ((prescaler + 1) * freq)) - 1;

	float smallestError = 1.0f;
	float calculatedFreq = 0;
	timParams_t tp;

	float temp;
	for (prescaler = 0; prescaler < 0xFF; prescaler++)
	{
		calculatedFreq = F_mcu / ((prescaler + 1) * (period[prescaler] + 1));
		temp = calculatedFreq - freq;

		if (temp < 0)
			temp *= -1;

		if (temp < smallestError)
		{
			smallestError = temp;
			tp.prescaler = prescaler;
			tp.period = period[prescaler];
		}
	}

	// assign calculated period and prescaller to the timer
	retVal += plcm_SetTimerParams(&tp, timer);

	assert_param(!retVal);
	return retVal;
}

/*
 * @brief	Assigns timer parameters to provided timer
 */
HAL_StatusTypeDef plcm_SetTimerParams(const timParams_t* const tp, TIM_HandleTypeDef* timer)
{
	assert_param(tp);
	assert_param(timer);

	timParams_t temp;
	temp.prescaler = timer->Init.Prescaler;
	temp.period = timer->Init.Period;

	timer->Init.Prescaler = tp->prescaler;
	timer->Init.Period = tp->period;

	if (HAL_TIM_Base_Init(timer)) // failure
	{
		timer->Init.Prescaler = temp.prescaler; // set previous params
		timer->Init.Period = temp.period;
		return HAL_ERROR;
	}

	return HAL_OK;
}

/*
 * @brief	Sets the output state of PLC OUTPUT PIN
 * @param	id: module identifier
 * @param	state: Non zero means state high
 */
void plcm_SetOutState(const uint32_t id, const uint32_t state)
{
	assert_param(id < plcIndex);
	HAL_GPIO_WritePin(plcm[id]->plcOutgpio, plcm[id]->plcOutpin, state);
}

/*
 * @brief	Toggles the PLC OUTPU pin state
 * @param	id: module identifier
 */
void plcm_TogOutState(const uint32_t id)
{
	assert_param(id < plcIndex);
	HAL_GPIO_TogglePin(plcm[id]->plcOutgpio, plcm[id]->plcOutpin);
}

/*
 * @brief	Reads the PLC INPUT state
 * @param	id: module identifier
 * @return	Non zero means high
 */
uint32_t plcm_GetInState(const uint32_t id)
{
	assert_param(id < plcIndex);
	return HAL_GPIO_ReadPin(plcm[id]->plcIngpio, plcm[id]->plcInpin);
}

/*
 * @brief	Reads the zero cross input state
 * @param	id: module identifier
 * @return	Non zero equals high state
 */
uint32_t plcm_GetZcState(const uint32_t id)
{
	assert_param(id < plcIndex);
	return HAL_GPIO_ReadPin(plcm[id]->zCgpio, plcm[id]->zCpin);
}

/*
 * @brief	Restarts the PLC sync timer module
 * @param	id: module identifier
 * @param	HAL_OK on success
 */
HAL_StatusTypeDef plcm_SyncTimerRestart(const uint32_t id)
{
	assert_param(id < plcIndex);
	HAL_StatusTypeDef retVal = HAL_OK;

	retVal += HAL_TIM_Base_Stop_IT(plcm[id]->timHandleSync); // turn off
	__HAL_TIM_CLEAR_IT(plcm[id]->timHandleSync, TIM_IT_UPDATE);
	plcm[id]->timHandleSync->Instance->CNT = 0; // reset counter
	retVal += HAL_TIM_Base_Start_IT(plcm[id]->timHandleSync); // turn on

	return retVal;
}

/*
 * @brief	Stops the PLC sync timer module
 * @param	id: module identifier
 * @param	HAL_OK on success
 */
HAL_StatusTypeDef plcm_SyncTimerStop(const uint32_t id)
{
	assert_param(id < plcIndex);
	HAL_StatusTypeDef retVal = HAL_OK;

	retVal += HAL_TIM_Base_Stop_IT(plcm[id]->timHandleSync); // turn off
	__HAL_TIM_CLEAR_IT(plcm[id]->timHandleSync, TIM_IT_UPDATE);
	plcm[id]->timHandleSync->Instance->CNT = 0; // reset counter

	return retVal;
}

/*
 * @brief	Restarts the PLC bits timer module
 * @param	id: module identifier
 * @param	HAL_OK on success
 */
HAL_StatusTypeDef plcm_BitsTimerRestart(const uint32_t id)
{
	assert_param(id < plcIndex);
	HAL_StatusTypeDef retVal = HAL_OK;

	retVal += HAL_TIM_Base_Stop_IT(plcm[id]->timHandleBits); // turn off
	__HAL_TIM_CLEAR_IT(plcm[id]->timHandleBits, TIM_IT_UPDATE);
	plcm[id]->timHandleBits->Instance->CNT = 0; // reset counter
	retVal += HAL_TIM_Base_Start_IT(plcm[id]->timHandleBits); // turn on

	return retVal;
}

/*
 * @brief	Stops the PLC bits timer module
 * @param	id: module identifier
 * @param	HAL_OK on success
 */
HAL_StatusTypeDef plcm_BitsTimerStop(const uint32_t id)
{
	assert_param(id < plcIndex);
	HAL_StatusTypeDef retVal = HAL_OK;

	retVal += HAL_TIM_Base_Stop_IT(plcm[id]->timHandleBits); // turn off
	__HAL_TIM_CLEAR_IT(plcm[id]->timHandleBits, TIM_IT_UPDATE);
	plcm[id]->timHandleBits->Instance->CNT = 0; // reset counter

	return retVal;
}

/*
 * @brief	Restarts the PLC ZC timer module
 * @param	id: module identifier
 * @param	HAL_OK on success
 */
HAL_StatusTypeDef plcm_ZcTimerRestart(const uint32_t id)
{
	assert_param(id < plcIndex);
	HAL_StatusTypeDef retVal = HAL_OK;

	retVal += HAL_TIM_Base_Stop_IT(plcm[id]->timHandleZc); // turn off
	__HAL_TIM_CLEAR_IT(plcm[id]->timHandleZc, TIM_IT_UPDATE);
	plcm[id]->timHandleZc->Instance->CNT = 0; // reset counter
	retVal += HAL_TIM_Base_Start_IT(plcm[id]->timHandleZc); // turn on

	return retVal;
}

/*
 * @brief	Checks either provided ZC pin matches any ZC pin in the modules table
 */
HAL_StatusTypeDef plcm_GetIdFromZcPin(uint16_t GPIO_Pin, uint32_t* id)
{
	assert_param(id);

	for (uint32_t i = 0; i < plcIndex; i++)
	{
		if (GPIO_Pin == plcm[i]->zCpin)
		{
			*id = i;
			return HAL_OK;
		}
	}

	return HAL_ERROR;
}

/*
 * @brief	Checks either provided In pin matches any In pin in the modules table
 */
HAL_StatusTypeDef plcm_GetIdFromInPin(uint16_t GPIO_Pin, uint32_t* id)
{
	assert_param(id);

	for (uint32_t i = 0; i < plcIndex; i++)
	{
		if (GPIO_Pin == plcm[i]->plcInpin)
		{
			*id = i;
			return HAL_OK;
		}
	}

	return HAL_ERROR;
}

/*
 * @brief	Checks Sync timer match
 */
HAL_StatusTypeDef plcm_GetIdFromSyncTim(TIM_HandleTypeDef* timer, uint32_t* id)
{
	assert_param(id);

	for (uint32_t i = 0; i < plcIndex; i++)
	{
		if (timer == plcm[i]->timHandleSync)
		{
			*id = i;
			return HAL_OK;
		}
	}

	return HAL_ERROR;
}

/*
 * @brief	Checks Bits timer match
 */
HAL_StatusTypeDef plcm_GetIdFromBitsTim(TIM_HandleTypeDef* timer, uint32_t* id)
{
	assert_param(id);

	for (uint32_t i = 0; i < plcIndex; i++)
	{
		if (timer == plcm[i]->timHandleBits)
		{
			*id = i;
			return HAL_OK;
		}
	}

	return HAL_ERROR;
}

HAL_StatusTypeDef plcm_GetIdFromZcTim(TIM_HandleTypeDef* timer, uint32_t* id)
{
	assert_param(id);

	for (uint32_t i = 0; i < plcIndex; i++)
	{
		if (timer == plcm[i]->timHandleZc)
		{
			*id = i;
			return HAL_OK;
		}
	}

	return HAL_ERROR;
}

HAL_StatusTypeDef plcm_GetIdFromPlc(plcm_t* p, uint32_t* id)
{
	assert_param(p);
	assert_param(id);

	for (uint32_t i = 0; i < plcIndex; i++)
	{
		if (p == plcm[i])
		{
			*id = i;
			return HAL_OK;
		}
	}

	return HAL_ERROR;
}

void plcm_StartSendAtNextSync(plcm_t* p)
{
	assert_param(p);
	uint32_t id;

	if (plcm_GetIdFromPlc(p, &id))
		return;

	// set bit timer to sending (1 ms)
	plcm_SetTimerParams(&p->timParamsBitsSend, p->timHandleBits);

	// restart the timer if master, slave already synced
	if (e_plcDevType_Master == plcm[id]->devType)
		plcm_SyncTimerRestart(id);

	// update state
	p->state = e_plcState_SendWaitForNextSyncTimeout;
	p->trans.status = e_plcTransStatus_Sending;
}

/*
 * @brief	Timer interrupt hook
 */
void plcm_timHandle(TIM_HandleTypeDef* timer)
{
	assert_param(timer);
	uint32_t id;

	if (!plcm_GetIdFromBitsTim(timer, &id)) // BITS
	{
		if (e_plcDevType_Slave == plcm[id]->devType)
			plcm_BitsTimerHandleSlave(id);
		else
			plcm_BitsTimerHandleMaster(id);
	}
	else if (!plcm_GetIdFromSyncTim(timer, &id)) // SYNC
	{
		if (e_plcDevType_Slave == plcm[id]->devType)
			plcm_SyncTimerHandleSlave(id);
		else
			plcm_SyncTimerHandleMaster(id);
	}
	else if (!plcm_GetIdFromZcTim(timer, &id)) // ZC tim
	{
		if (e_plcDevType_Master == plcm[id]->devType)
			plcm_ZcTimerHandleMaster(id);
	}
}

void plcm_BitsTimerHandleMaster(const uint32_t id)
{
	plcm_BitsTimerHandleSlave(id); // same functionality
}

void plcm_SyncTimerHandleMaster(const uint32_t id)
{
	switch (plcm[id]->state)
	{
		case e_plcState_SendWaitForNextSyncTimeout:
		{
			plcm_SendSyncBit(id); // send sync
			break;
		}

		default:
		{
			// do nothing
		}
	}
}

void plcm_SyncTimerHandleSlave(const uint32_t id)
{
	switch (plcm[id]->state)
	{
		case e_plcState_SendWaitForNextSyncTimeout:
		{
			plcm_SendSyncBit(id); // send sync
			break;
		}

		default:
		{
			// do nothing
		}
	}
}

void plcm_BitsTimerHandleSlave(const uint32_t id)
{
	switch (plcm[id]->state)
	{
		// RECV

		case e_plcState_RecvWaitForSyncBitTimeout:
		{
			plcm_ReadDataBit(id); // read the data bit
			plcm_BitsTimerStop(id); // stop bit timer
			break;
		}

		// SEND

		case e_plcState_SendWaitForSyncBitDone:
		{
			// now set data bit
			plcm_SendDataBit(id);
			break;
		}

		case e_plcState_SendWaitForDataBitDone:
		{
			plcm_FinishSendChunk(id);

			// set high state on tx line, wait for next sync if still sending
			switch (plcm[id]->trans.status)
			{
				case e_plcTransStatus_Sending:
				{
					plcm[id]->state = e_plcState_SendWaitForNextSyncTimeout;
					break;
				}

				default:
				{
					// debug
				}
			}
			break;
		}

		default:
		{
			// add debug leds
		}
	}
}

void plcm_ZcTimerHandleMaster(const uint32_t id)
{
	// increments ZC counter. If it reaches a certain amount of expirations value, set DC
	if (plcm[id]->noZcCounter < PLC_EXP_TIL_DC)
	{
		plcm[id]->supply = e_plcSupplyType_Ac;
		plcm[id]->noZcCounter++;
	}
	else
		plcm[id]->supply = e_plcSupplyType_Dc;
}

/*
 * @brief	External interrupt hook
 */
void plcm_extiHandle(uint16_t GPIO_Pin)
{
	assert_param(GPIO_Pin);
	uint32_t id;

	if (HAL_OK == plcm_GetIdFromZcPin(GPIO_Pin, &id)) // ZC interrupt, only for master
	{
		if (plcm[id]->strobe != plcm_GetZcState(id)) // react only on low strobe
			return;

		// at this point ZC pin is identified and edge is falling
		plcm[id]->noZcCounter = 0;

		switch (plcm[id]->state)
		{
			case e_plcState_SendWaitForNextZc:
			{
				// start the transmission timer
				plcm_StartSendAtNextSync(plcm[id]);
				break;
			}

			case e_plcState_Idle:
			{
				// expiration in idle state can only mean AC is on
				//plcm_SyncTimerRestart(id);
				break;
			}

			default:
			{
				// add logging leds
			}
		}
	}

	else if (HAL_OK == plcm_GetIdFromInPin(GPIO_Pin, &id))
	{
		// at this point IN pin rising edge is identified, check if in reception mode
		if (e_plcTransStatus_Receiving != plcm[id]->trans.status)
			return;

		if (e_plcDevType_Slave == plcm[id]->devType)
			plcm_InPinHandleSlave(id);
		else
			plcm_InPinHandleMaster(id);
	}
}

void plcm_InPinHandleSlave(const uint32_t id)
{
	switch (plcm[id]->state)
	{
		case e_plcState_RecvWaitForSyncBit1st:
		{
			// this is 1st sync bit. Master doesnt care, slave starts sync timer
			if (e_plcDevType_Slave == plcm[id]->devType)
				plcm_SyncTimerRestart(id);

			// set bit timer to recv timing and start it
			plcm_SetTimerParams(&plcm[id]->timParamsBitsRecv, plcm[id]->timHandleBits);
			plcm_BitsTimerRestart(id);

			// update state
			plcm[id]->state = e_plcState_RecvWaitForSyncBitTimeout;
			break;
		}

		case e_plcState_RecvWaitForSyncBit:
		{
			// same as 1st, but sync timer is already set and started
			plcm_BitsTimerRestart(id);
			plcm[id]->state = e_plcState_RecvWaitForSyncBitTimeout;
			break;
		}

		default:
		{
			// add logging leds
		}
	}
}

void plcm_InPinHandleMaster(const uint32_t id)
{
	plcm_InPinHandleSlave(id); // same functionality for now
}

/*
 * @brief	Sets the out state to high to indicate sync bit. Starts the bit timer to
 * 			set the data bit value in next timer ISR.
 * @param	id: plc module identifier
 * @return	HAL_OK if timer restarted succesfully.
 */
HAL_StatusTypeDef plcm_SendSyncBit(const uint32_t id)
{
	// set sync strobe
	plcm_SetOutState(id, 0);

	// set machine state
	plcm[id]->state = e_plcState_SendWaitForSyncBitDone;

	// start timer to measure sync bit len
	return plcm_BitsTimerRestart(id);
}

/*
 * @brief	Sets the out state to the value corresponding to the current data bit.
 * 			After setting the last bit, changes state to recv/ idle.
 * @param	id: plc module identifier
 */
void plcm_SendDataBit(const uint32_t id)
{
	// get the bit state
	uint32_t bit = plcm[id]->trans.data[plcm[id]->trans.byteIndex] &
			(1 << plcm[id]->trans.bitIndex);

	// set data strobe
	plcm_SetOutState(id, !bit);

	// increment counters
	// bit
	if (plcm[id]->trans.bitIndex >= 7)
	{
		plcm[id]->trans.bitIndex = 0;

		// byte
		if (plcm[id]->trans.byteIndex >= (plcm[id]->trans.len - 1)) // end transmission
		{
			plcm_resetTransData(plcm[id]);

			// transmission finished
			plcm_SyncTimerStop(id);

			// call tx routine
			plcm_txRoutine(plcm[id]);

			// reception now, wait for 1st sync bit
			plcm[id]->state = e_plcState_RecvWaitForSyncBit1st;
			return;
		}
		else
			plcm[id]->trans.byteIndex++;
	}
	else
		plcm[id]->trans.bitIndex++;

	plcm[id]->state = e_plcState_SendWaitForDataBitDone;
}

/*
 * @brief	Reads the In state bit.
 * 			After setting the last bit, changes state to recv/ idle.
 * @param	id: plc module identifier
 */
void plcm_ReadDataBit(const uint32_t id)
{
	// save bit value
	uint32_t bit = plcm_GetInState(id) ? 0 : 1; // 0s on PLC are 1s

	// clear whole byte at first bit
	if (!plcm[id]->trans.bitIndex)
		*plcm[id]->trans.data = 0;

	// append bit
	*plcm[id]->trans.data |= bit << plcm[id]->trans.bitIndex;

	if (plcm[id]->trans.bitIndex >= 7)
	{
		plcm[id]->trans.bitIndex = 0;

		// inform that byte is obtained
		plcm_rxRoutine(plcm[id]);
	}
	else
		plcm[id]->trans.bitIndex++;

	plcm[id]->state = e_plcState_RecvWaitForSyncBit;
}

/*
 * @brief	Restarts all counters and timers. Puts the module into default idle state
 */
HAL_StatusTypeDef plcm_SetIdleState(const uint32_t id)
{
	HAL_StatusTypeDef retVal = HAL_OK;

	// out state high (idle)
	plcm_SetOutState(id, 1);

	// Sync timer: stop it, set it do searching for AC, restart it
	retVal += plcm_SyncTimerStop(id);
	//retVal += plcm_SetTimerParams(&plcm[id]->timParamsSyncRecv, plcm[id]->timHandleSync);
	//retVal += plcm_SyncTimerRestart(id);

	// Bits timer: set it to send if master, stop it
	timParams_t* tp = (e_plcDevType_Master == plcm[id]->devType) ?
			&plcm[id]->timParamsBitsSend : &plcm[id]->timParamsBitsRecv;

	retVal += plcm_SetTimerParams(tp, plcm[id]->timHandleBits);
	retVal += plcm_BitsTimerStop(id);

	// update state
	if (e_plcDevType_Master == plcm[id]->devType)
		plcm[id]->state = e_plcState_Idle;
	else
		plcm[id]->state = e_plcState_RecvWaitForSyncBit1st;

	// reset transmission data
	plcm_resetTransData(plcm[id]);

	return retVal;
}

/*
 * @brief	Restarts all transfer/ receive parameters
 */
void plcm_resetTransData(plcm_t* plc)
{
	assert_param(plc);

	plc->trans.status = e_plcTransStatus_Receiving;
	plc->trans.data = &plc->recvByte;
	plc->trans.byteIndex = 0;
	plc->trans.bitIndex = 0;
	plc->trans.len = 0;
}

/*
 * @brief	Puts tx line in low state, stops bits timer and waits for next sync
 */
HAL_StatusTypeDef plcm_FinishSendChunk(const uint32_t id)
{
	plcm_SetOutState(id, 1);
	return plcm_BitsTimerStop(id);
}



// OVERRIDES

/*
 * @brief	Weak override for plc transmit complete
 * @param	plcHandle: plc module that just completed transmission
 */
__attribute__((weak)) void plcm_txRoutine(plcm_t* plcHandle) { }

/*
 * @brief	Weak override for plc receive complete
 * @param	plcHandle: plc module that just completed reception
 */
__attribute__((weak)) void plcm_rxRoutine(plcm_t* plcHandle) { }
















