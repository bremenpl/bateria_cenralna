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
uint16_t mbg_CalculateCrc2(uint8_t* data, size_t len);
HAL_StatusTypeDef mbg_CheckSendStatus(UART_HandleTypeDef* uHandle);
HAL_StatusTypeDef mbg_SendData(mbgUart_t* uart);

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

uint16_t mbg_CalculateCrc2(uint8_t* data, size_t len)
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
	uint16_t calcCrc = mbg_CalculateCrc2(buf, i);

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

	/*
	 * TODO: Initialize the serial parameters here maybe?
	 */

	// Init send mutex
	osMutexDef_t tempMutDef;
	uart->txMut = osMutexCreate(&tempMutDef);
	if (!uart->txMut)
		retVal++;

	// init frames buffer
	uart->rxQ.framesIndex = 0;

	if (!uart->rxQ.framesBufLen)
		retVal++;
	else
	{
		uart->rxQ.frames = malloc(sizeof(mbgFrame_t) * uart->rxQ.framesBufLen);
		if (!uart->rxQ.frames)
			retVal++;
	}

	return retVal;
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
	{
		// check if rx timeout is disabled, otherwise turning DMA off might
		// cut the tx transfer process
		if (!(uHandle->Instance->CR2 & USART_CR2_RTOEN))
			return HAL_OK;
	}

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
	uart->buf[uart->len++] = mf->addr;

	// func code
	uart->buf[uart->len++] = mf->code;

	// data
	uint32_t k;
	for (k = 0; k < mf->dataLen; k++, uart->len++)
		uart->buf[uart->len] = mf->data[k];

	// Calculate crc
	mf->crc = mbg_CalculateCrc2(uart->buf, uart->len);

	// add the crc to the buffer (inverse order)
	uart->buf[uart->len++] = (uint8_t)mf->crc;
	uart->buf[uart->len++] = (uint8_t)(mf->crc >> 8);

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
	if (!uart)
		return HAL_ERROR;

	if (!uart->len)
		return HAL_OK; // nothing to send

	HAL_StatusTypeDef retVal = HAL_OK;

	// lock
	osMutexWait(uart->txMut, osWaitForever);

	// send with DMA
	while (mbg_CheckSendStatus(uart->handle))
		osDelay(1);

	// disable receiver
	mbg_EnableReceiver(uart->handle, 0, 0, 0);

	retVal += HAL_UART_Transmit_DMA(uart->handle, uart->buf, uart->len);

	// release
	osMutexRelease(uart->txMut);
	return retVal;
}

/*
 * @brief	Enables the MODBUS t3,5 char timer.
 * @param	uHandle: pointer to an uart handle struct.
 */
void mbg_EnableRxTimeout(UART_HandleTypeDef* uHandle)
{
	assert_param(uHandle);

	// manually initialize registers for t3.5 timing
	//mbg_DisableRxTimeout(uHandle);

	uHandle->Instance->CR2 |= USART_CR2_RTOEN; // Receiver timeout enable
	uHandle->Instance->CR1 |= USART_CR1_RTOIE; // Receiver timeout interrupt enable
	uHandle->Instance->RTOR |= 100;			   // Receiver timeout value (11 * 3.5)
}

/*
 * @brief	Disables the MODBUS t3,5 char timer.
 * @param	uHandle: pointer to an uart handle struct.
 */
void mbg_DisableRxTimeout(UART_HandleTypeDef* uHandle)
{
	assert_param(uHandle);

	uHandle->Instance->CR2 &= ~USART_CR2_RTOEN; // Receiver timeout disable
	uHandle->Instance->CR1 &= ~USART_CR1_RTOIE; // Receiver timeout interrupt disable
	__HAL_UART_CLEAR_IT(uHandle, UART_CLEAR_RTOCF); // clear RTOF flag
}

/*
 * @brief	Enable half duplex uart DMA receiver for \ref len number of bytes.
 * @param	uHandle: uart handle struct pointer.
 * @param	data: pointer to data stream.
 * @param	len: amount of bytes to receive.
 * @param	enableRxTimeout: Non zero means rx timeout should be ON.
 * @return	HAL_OK if inits succesfull.
 */
HAL_StatusTypeDef mbg_EnableReceiver(UART_HandleTypeDef* uHandle,
		uint8_t* data, const uint16_t len, const uint32_t enableRxTimeout)
{
	assert_param(uHandle);
	HAL_StatusTypeDef retVal = HAL_OK;

	// Disable RX timeout interrupt
	mbg_DisableRxTimeout(uHandle);

	// Disable uart RX DMA
	retVal += HAL_UART_DMAStop(uHandle);

	// Wait for ready status
	while ((HAL_UART_GetState(uHandle) != HAL_UART_STATE_READY) &&
			(HAL_UART_GetState(uHandle) != HAL_UART_STATE_BUSY_TX));

	// Enable receiver
	if (len && data)
	{
		SET_BIT(uHandle->Instance->CR1, USART_CR1_RE);
		retVal += HAL_UART_Receive_DMA(uHandle, data, len);
	}
	else // disable receiving
		CLEAR_BIT(uHandle->Instance->CR1, USART_CR1_RE);

	// Enable Rx timeout if rx DMA is ok and parameter is set
	if ((!retVal) && enableRxTimeout)
		mbg_EnableRxTimeout(uHandle);

	return retVal;
}

/*
 * @brief	Saves 96 bit unique id under \ref id pointer
 */
void mbg_getUniqId(uint16_t* id)
{
	HAL_FLASH_Unlock();

	for (uint32_t i = 0; i < UNIQ_ID_REGS; i++)
	{
		assert_param(id + i);
		id[i] = (*(__IO uint16_t*)(UNIQ_ID_ADDR + (i * 2)));
	}

	HAL_FLASH_Lock();
}

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
 * @brief	Place this function in the \ref USART1_IRQHandler handler because HAL
 * 			doesnt support RTOCF flag clearing.
 * @param	uHandle: uart handle struct pointer.
 */
void mbg_ClearRTOCF_Flag(UART_HandleTypeDef* uHandle)
{
	assert_param(uHandle);

	// check RTOF flag
	if((__HAL_UART_GET_IT(uHandle, UART_IT_RTOF) != RESET) &&
		  (__HAL_UART_GET_IT_SOURCE(uHandle, UART_IT_RTOF) != RESET))
	{
		__HAL_UART_CLEAR_IT(uHandle, UART_CLEAR_RTOCF);

		// Handle this situation differently in slave / master module
		mbs_uartRxTimeoutRoutine(uHandle);
		mbm_uartRxTimeoutRoutine(uHandle);
	}
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
 * @brief	ModBus MASTER weak override function. Do NOT override it in SLAVE framework.
 * @param	uHandle: pointer to the uart modbus master struct.
 */
__attribute__((weak)) void mbs_uartRxTimeoutRoutine(UART_HandleTypeDef* uHandle) { }

/*
 * @brief	ModBus SLAVE weak override function. Do NOT override it in MASTER framework.
 * @param	uHandle: pointer to the uart modbus slave struct.
 */
__attribute__((weak)) void mbm_uartRxTimeoutRoutine(UART_HandleTypeDef* uHandle) { }

/*
 * @brief	ModBus GENERIC weak override function. It can be overriden in any module
 * 			(master or slave). Use it to print specific messages using your custom logger
 * 			each time a new message is sent or received (request or response).
 * @param	mf: pointer to the received/ sent struct.
 */
__attribute__((weak)) void mbg_uartPrintFrame(const mbgFrame_t* const mf)  { }




