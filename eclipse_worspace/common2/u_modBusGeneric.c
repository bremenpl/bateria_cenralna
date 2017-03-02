/*
 * u_modBusGeneric.c
 *
 *  Created on: 14 lis 2016
 *      Author: Lukasz
 */



/* Includes ------------------------------------------------------------------*/
#include "u_modBusGeneric.h"
#include <stdlib.h>

/* Defines and macros --------------------------------------------------------*/

/* Enums and structs ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

// IMPORTANT: Modbus generic module should not have any private variables.

/* Public variables ----------------------------------------------------------*/

/* Fuction prototypes --------------------------------------------------------*/
uint16_t mbg_CalculateCrc(uint8_t* data, size_t len);
HAL_StatusTypeDef mbg_CheckSendStatus(UART_HandleTypeDef* uHandle);
HAL_StatusTypeDef mbg_SendData(mbgUart_t* uart);
HAL_StatusTypeDef mbg_CalcRxTimeout(UART_HandleTypeDef* uart,
									TIM_HandleTypeDef* tim,
									float T_35);
HAL_StatusTypeDef mbg_ResetRxTimer(mbgUart_t* uart);
HAL_StatusTypeDef mbg_DisableRxTimer(mbgUart_t* uart);
HAL_StatusTypeDef mbg_EnableReceiver(mbgUart_t* uart);


/* Function declarations -----------------------------------------------------*/

inline static uint16_t crc_reflect(uint16_t data, uint32_t len)
{
	// Generated by pycrc v0.8.3, https://pycrc.org
	// Width = 16, Poly = 0x8005, XorIn = 0xffff, ReflectIn = True,
	// XorOut = 0x0000, ReflectOut = True, Algorithm = bit-by-bit-fast

	uint16_t ret = data & 0x01;

	for (int32_t i = 1; i < len; i++)
	{
		data >>= 1;
		ret = (ret << 1) | (data & 0x01);
	}

	return ret;
}

uint16_t mbg_CalculateCrc(uint8_t* data, size_t len)
{
	// Generated by pycrc v0.8.3, https://pycrc.org
	// Width = 16, Poly = 0x8005, XorIn = 0xffff, ReflectIn = True,
	// XorOut = 0x0000, ReflectOut = True, Algorithm = bit-by-bit-fast

	uint16_t crc = 0xFFFF;

	while (len--)
	{
		const uint8_t c = *data++;

		for (int32_t i = 0x01; i & 0xFF; i <<= 1)
		{
			bool bit = crc & 0x8000;
			if (c & i)
				bit = !bit;

			crc <<= 1;
			if (bit)
				crc ^= 0x8005;
		}

		crc &= 0xFFFF;
	}

	return crc_reflect(crc & 0xFFFF, 16) ^ 0x0000;
}

/*
 * @brief	Function calculates the crc of provided \ref mf frame
 * 			and compares it to the crc of it.
 * @param	mf: pointer to a modbus frame.
 * @param	crc: under this pointer calculated crc is saved if address is not 0
 * @return	HAL_OK if calculated crc and \ref mf crc match.
 */
HAL_StatusTypeDef mbg_CheckCrc(const mbgFrame_t* const mf, uint16_t* crc)
{
	if (!mf)
		return HAL_ERROR;

	// prepare the data, copy it for the simplicity sake.
	// It is done in a non blocking thread anyways.
	uint32_t i = 0;
	uint8_t buf[MBG_MAX_FRAME_LEN];

	// addr
	buf[i++] = mf->addr;

	// function code
	buf[i++] = mf->code;

	// data
	uint32_t k;
	for (k = 0; k < mf->dataLen; k++, i++)
		buf[i] = mf->data[k];

	// Calculate crc
	uint16_t calcCrc = mbg_CalculateCrc(buf, i);
	calcCrc = (calcCrc >> 8) | (calcCrc << 8); // flip

	// save calculated crc
	if (crc)
		*crc = calcCrc;

	// check match
	if (calcCrc != mf->crc)
		return HAL_ERROR;

	return HAL_OK;;
}

/*
 * @brief	Initializes the common hardware part for MODBUS master/ slave.
 * 			Run it inside master/ slave init function.
 * @param	uart: Uart handle pointer.
 * @param	HAL_OK if initialization complete.
 */
HAL_StatusTypeDef mbg_UartInit(mbgUart_t* uart)
{
	assert_param(uart);
	if (uart == NULL)
		return HAL_ERROR;

	HAL_StatusTypeDef retVal = HAL_OK;

	// Calculate Rx timeout timer overflow value
	retVal += mbg_CalcRxTimeout(uart->handle, uart->rxQ.toutTim, uart->rxQ.T35);

	// trigger out the init interrupt, enable receiver
	retVal += mbg_ResetRxTimer(uart);
	retVal += mbg_DisableRxTimer(uart);
	retVal += mbg_DisableReceiver(uart);
	retVal += mbg_EnableReceiver(uart);

	return retVal;
}

/*
 * @brief	New rx bytes enqueue function.
 * @param	uart: pointer to generic modbus struct
 * @return	HAL_OK if added correctly to the queue
 */
HAL_StatusTypeDef mbg_ByteReceived(mbgUart_t* uart)
{
	assert_param(uart);
	HAL_StatusTypeDef retVal = HAL_OK;

	// try to enqueue the byte
	if (osOK != osMessagePut(uart->rxQ.msgQId, (uint32_t)uart->rxQ.rxByte, osWaitForever))
		return HAL_OK;

	// turn on the rx interrupts again, reset the timer
	retVal += mbg_ResetRxTimer(uart);
	retVal += mbg_EnableReceiver(uart);

	return retVal;
}

/*
 * @brief	Function should be called when RX timeout occurs
 * @param	uart: pointer to generic modbus struct
 * @return	HAL_OK if timeout routine was correct
 */
HAL_StatusTypeDef mbg_RxTimeout(mbgUart_t* uart)
{
	assert_param(uart);
	assert_param(uart->rxQ.toutTim);
	HAL_StatusTypeDef retVal = HAL_OK;

	// turn off the timer
	retVal += mbg_DisableRxTimer(uart);
	retVal += mbg_DisableReceiver(uart);
	retVal += mbg_EnableReceiver(uart);

	// reset state
	uart->rxState = e_mbgRxState_addr;
	return HAL_OK;
}

/*
 * @brief	Check if sending through \ref uHandle is possible.
 * @param	uHandle: pointer to a uart struct.
 * @return	HAL_OK if can send.
 */
HAL_StatusTypeDef mbg_CheckSendStatus(UART_HandleTypeDef* uHandle)
{
	HAL_UART_StateTypeDef state = HAL_UART_GetState(uHandle);
	if ((state == HAL_UART_STATE_READY) || (state == HAL_UART_STATE_BUSY_RX))
		return HAL_OK;

	return HAL_ERROR;
}

/*
 * @brief	Sends the specified \ref mf through \ref uHandle.
 * 			Also calculates the CRC for the frame.
 * @param	uart: uart handler struct pointer.
 * @param	mf: frame to be sent pointer.
 * @return	HAL_OK if sent succesfull.
 */
HAL_StatusTypeDef mbg_SendFrame(mbgUart_t* uart, mbgFrame_t* mf)
{
	if (!uart)
		return HAL_ERROR;

	// start stuffing the buffer with data
	uart->len = 0;

	// address
	uart->txbuf[uart->len++] = mf->addr;

	// func code
	uart->txbuf[uart->len++] = mf->code;

	// data
	uint32_t k;
	for (k = 0; k < mf->dataLen; k++, uart->len++)
		uart->txbuf[uart->len] = mf->data[k];

	// Calculate crc
	mf->crc = mbg_CalculateCrc(uart->txbuf, uart->len);

	// add the crc to the buffer (inverse order)
	uart->txbuf[uart->len++] = (uint8_t)mf->crc;
	uart->txbuf[uart->len++] = (uint8_t)(mf->crc >> 8);

	// send the data and return
	HAL_StatusTypeDef retVal = mbg_SendData(uart);

	// print the frame
	if (!retVal)
		mbg_uartPrintFrame(mf);

	return retVal;
}

/*
 * @brief	Sends specified \ref data of size \ref len, using \ref uHandle.
 * 			Data is transmitted in half duplex mode.
 * @param	uart: uart handle using which data will be sent.
 * @return	HAL_OK if transmission succesfull.
 */
HAL_StatusTypeDef mbg_SendData(mbgUart_t* uart)
{
	assert_param(uart);

	// disable receiving
	mbg_DisableReceiver(uart);

	if (!uart->len)
		return HAL_OK; // nothing to send

	HAL_StatusTypeDef retVal = HAL_OK;

	// send with DMA
	while (mbg_CheckSendStatus(uart->handle))
		osDelay(1);

	retVal += HAL_UART_Transmit_DMA(uart->handle, uart->txbuf, uart->len);
	return retVal;
}

/*
 * @brief	Calculates the \ref tim period and prescaller (taking under
 * 			consideration \ref uart parameters) in a way that a timer interrupt
 * 			will be triggered if the time of \ref signTenParts will pass.
 * @param	uart: uart struct for which timeout is configured.
 * @param	tim: 16 bit timeout timer handle struct.
 * @param	T35: signs for timeout, 3.5 in default
 */
HAL_StatusTypeDef mbg_CalcRxTimeout(UART_HandleTypeDef* uart,
									TIM_HandleTypeDef* tim,
									float T35)
{
	assert_param(tim);
	assert_param(uart);
	assert_param(T35 != 0); // cant be 0

	HAL_StatusTypeDef retVal = HAL_OK;
	float F_mcu = HAL_RCC_GetHCLKFreq();
	float baud = uart->Init.BaudRate;
	float bits = 10; // 8 data bits + 1 start bit + 1 stop bit

	// find out if need to extend bits
	if (uart->Init.Parity != UART_PARITY_NONE)
		bits++;

	if (uart->Init.StopBits == UART_STOPBITS_2)
		bits++;

	float bytesPerSec = baud / bits;
	float F_mod = bytesPerSec / T35;

	// now find proper prescaller and period for \ref tim
	uint16_t period[0xFF];
	uint32_t prescaler = 0;

	for (prescaler = 0; prescaler < 0xFF; prescaler++)
		period[prescaler] = (F_mcu / ((prescaler + 1) * F_mod)) - 1;

	float smallestError = 1.0f;
	float calculatedFreq = 0;

	uint32_t finalPrescaler = 0;
	uint32_t finalPeriod = 0;

	for (prescaler = 0; prescaler < 0xFF; prescaler++)
	{
		calculatedFreq = F_mcu / ((prescaler + 1) * (period[prescaler] + 1));
		float temp = calculatedFreq - F_mod;

		if (temp < 0)
			temp *= -1;

		if (temp < smallestError)
		{
			smallestError = temp;
			finalPrescaler = prescaler;
			finalPeriod = period[prescaler];
		}
	}

	// assign calculated period and prescaller to the timer
	tim->Init.Prescaler = finalPrescaler;
	tim->Init.Period = finalPeriod;
	retVal += HAL_TIM_Base_Init(tim);

	assert_param(!retVal);
	return retVal;
}

/*
 * @brief	Resets the rx timeout timer for specified generic struct
 * @param	uart: generic struct pointer
 * @return	HAL_OK if all ok.
 */
HAL_StatusTypeDef mbg_ResetRxTimer(mbgUart_t* uart)
{
	assert_param(uart);
	assert_param(uart->rxQ.toutTim);
	HAL_StatusTypeDef retVal = HAL_OK;

	retVal += HAL_TIM_Base_Stop_IT(uart->rxQ.toutTim); // turn off
	__HAL_TIM_CLEAR_IT(uart->rxQ.toutTim, TIM_IT_UPDATE);
	uart->rxQ.toutTim->Instance->CNT = 0; // reset counter
	retVal += HAL_TIM_Base_Start_IT(uart->rxQ.toutTim); // turn on

	return retVal;
}

/*
 * @brief	Turns off the rx timeout timer for specified generic struct
 * @param	uart: generic struct pointer
 * @return	HAL_OK if all ok.
 */
HAL_StatusTypeDef mbg_DisableRxTimer(mbgUart_t* uart)
{
	assert_param(uart);
	assert_param(uart->rxQ.toutTim);
	HAL_StatusTypeDef retVal = HAL_OK;

	retVal += HAL_TIM_Base_Stop_IT(uart->rxQ.toutTim); // turn off
	__HAL_TIM_CLEAR_IT(uart->rxQ.toutTim, TIM_IT_UPDATE);
	uart->rxQ.toutTim->Instance->CNT = 0; // reset counter

	return retVal;
}

/*
 * @brief	Enabled the 1 byte interrupt receiver
 * @param	uart: generic struct pointer
 * @return	HAL_OK if all ok.
 */
HAL_StatusTypeDef mbg_EnableReceiver(mbgUart_t* uart)
{
	assert_param(uart);
	assert_param(uart->handle);
	HAL_StatusTypeDef retVal = HAL_OK;

	// enable receiving
	SET_BIT(uart->handle->Instance->CR1, USART_CR1_RE);
	retVal += HAL_UART_Receive_IT(uart->handle, &uart->rxQ.rxByte, 1);

	return retVal;
}

/*
 * @brief	Disable receiving. Use when transmitting
 * @param	uart: generic struct pointer
 * @return	HAL_OK if all ok.
 */
HAL_StatusTypeDef mbg_DisableReceiver(mbgUart_t* uart)
{
	assert_param(uart);
	assert_param(uart->handle);
	HAL_StatusTypeDef retVal = HAL_OK;

	// turn off rx timeout
	retVal += mbg_DisableRxTimer(uart);

	// disable receiving
	CLEAR_BIT(uart->handle->Instance->CR1, USART_CR1_RE);
	retVal += HAL_UART_AbortReceive(uart->handle);

	return retVal;
}

/*
 * @brief	Determine whether we are in thread mode or handler mode.
 * @return	non zero if in handler mode
 */
int mbg_inHandlerMode()
{
	return (__get_IPSR() != 0);
}

/*
 * @brief	Saves 96 bit unique id under \ref id pointer
 */
/*void mbg_getUniqId(uint16_t* id)
{
	HAL_FLASH_Unlock();

	for (uint32_t i = 0; i < UNIQ_ID_REGS; i++)
	{
		assert_param(id + i);
		id[i] = (*(__IO uint16_t*)(UNIQ_ID_ADDR + (i * 2)));
	}

	HAL_FLASH_Lock();
}*/

/*
 * @brief	Uart TX complete handle overwrite function for both master and slave modules.
 * 			Depending on which driver is used the proper function has to be
 * 			overwritten in master/ slave module.
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	// check if SLAVE handled
	mbs_uartTxRoutine(huart);

	// check if MASTER handled
	mbm_uartTxRoutine(huart);
}

/*
 * @brief	Uart RX complete handle overwrite function for both master and slave modules.
 * 			Depending on which driver is used the proper function has to be
 * 			overwritten in master/ slave module.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	// check if SLAVE handled
	mbs_uartRxRoutine(huart);

	// check if MASTER handled
	mbm_uartRxRoutine(huart);
}

/*
 * @brief	Uart RX timeout handle overwrite function for both master and slave modules.
 * 			Depending on which driver is used the proper function has to be
 * 			overwritten in master/ slave module.
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	// check if SLAVE handled
	mbs_uartRxTimeoutRoutine(htim);

	// check if MASTER handled
	mbm_uartRxTimeoutRoutine(htim);
}

/*
 * @brief	ModBus SLAVE weak override function. Do NOT override it in MASTER framework.
 * @param	uHandle: pointer to the uart modbus master struct.
 */
__attribute__((weak)) void mbs_uartRxRoutine(UART_HandleTypeDef* uHandle) { }

/*
 * @brief	ModBus MASTER weak override function. Do NOT override it in SLAVE framework.
 * @param	uHandle: pointer to the uart modbus slave struct.
 */
__attribute__((weak)) void mbm_uartRxRoutine(UART_HandleTypeDef* uHandle) { }

/*
 * @brief	ModBus SLAVE weak override function. Do NOT override it in MASTER framework.
 * @param	uHandle: pointer to the uart modbus master struct.
 */
__attribute__((weak)) void mbs_uartTxRoutine(UART_HandleTypeDef* uHandle) { }

/*
 * @brief	ModBus MASTER weak override function. Do NOT override it in SLAVE framework.
 * @param	uHandle: pointer to the uart modbus slave struct.
 */
__attribute__((weak)) void mbm_uartTxRoutine(UART_HandleTypeDef* uHandle) { }

/*
 * @brief	ModBus SLAVE weak override function. Do NOT override it in SLAVE framework.
 * @param	uHandle: pointer to the uart modbus master struct.
 */
__attribute__((weak)) void mbs_uartRxTimeoutRoutine(TIM_HandleTypeDef* tim) { }

/*
 * @brief	ModBus SLAVE weak override function. Do NOT override it in MASTER framework.
 * @param	uHandle: pointer to the uart modbus slave struct.
 */
__attribute__((weak)) void mbm_uartRxTimeoutRoutine(TIM_HandleTypeDef* tim) { }

/*
 * @brief	ModBus GENERIC weak override function. It can be overriden in any module
 * 			(master or slave). Use it to print specific messages using your custom logger
 * 			each time a new message is sent or received (request or response).
 * @param	mf: pointer to the received/ sent struct.
 */
__attribute__((weak)) void mbg_uartPrintFrame(const mbgFrame_t* const mf)  { }




