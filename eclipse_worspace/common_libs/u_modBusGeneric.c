/*
 * u_modBusGeneric.c
 *
 *  Created on: 14 lis 2016
 *      Author: Lukasz
 */



/* Includes ------------------------------------------------------------------*/
#include "u_modBusGeneric.h"

/* Defines and macros --------------------------------------------------------*/

/* Enums and structs ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static osMutexId mbg_sendMut;	/*!< Used for allowing sending only in one thread */
static uint8_t mbg_rxTxBuffer[MBG_MAX_FRAME_LEN];

/* Public variables ----------------------------------------------------------*/

/* Fuction prototypes --------------------------------------------------------*/
uint16_t mbg_CalculateCrc(uint8_t* data, size_t len);
HAL_StatusTypeDef mbg_CheckSendStatus(UART_HandleTypeDef* uHandle);
HAL_StatusTypeDef mbg_SendData(UART_HandleTypeDef* uHandle, uint8_t* data, size_t len);

/* Function declarations -----------------------------------------------------*/

/*
 * @brief	Function returns CRC calculated from provided \ref data of \ref len.
 * @param	data: pointer to the data stream.
 * @param	len: length of the data.
 * @return	Calculated CRC.
 */
uint16_t mbg_CalculateCrc(uint8_t* data, size_t len)
{
	/* Table of CRC values for high–order byte */
	static const uint8_t auchCRCHi[] =
	{
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
	0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
	0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
	0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
	0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
	0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
	0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
	0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
	0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
	0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
	0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
	0x40
	};

	/* Table of CRC values for low–order byte */
	static const uint8_t auchCRCLo[] =
	{
	0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4,
	0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
	0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD,
	0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
	0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7,
	0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
	0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE,
	0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
	0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2,
	0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
	0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB,
	0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
	0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91,
	0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
	0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88,
	0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
	0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80,
	0x40
	};

	assert_param(data);

	uint8_t uchCRCHi = 0xFF ; 			/* high byte of CRC initialized */
	uint8_t uchCRCLo = 0xFF ; 			/* low byte of CRC initialized */
	uint32_t uIndex ; 					/* will index into CRC lookup table */

	while (len--) 						/* pass through message buffer */
	{
		uIndex = uchCRCLo ^ *data++ ; 	/* calculate the CRC */
		uchCRCLo = uchCRCHi ^ auchCRCHi[uIndex] ;
		uchCRCHi = auchCRCLo[uIndex] ;
	}

	uint16_t crc = ((uint16_t)uchCRCHi << 8) | (uint16_t)uchCRCLo;
	return  crc;
}

/*
 * @brief	Function calculates the crc of provided \ref mf frame
 * 			and compares it to the crc of it.
 * @param	mf: pointer to a modbus frame.
 * @return	HAL_OK if calculated crc and \ref mf crc match.
 */
HAL_StatusTypeDef mbg_CheckCrc(const modbusFrame_t* const mf)
{
	assert_param(mf);
	HAL_StatusTypeDef retVal = HAL_OK;

	// prepare the data, copy it for the simplicity sake. It is done in a non blocking
	// thread anyways.
	uint32_t i = 0;

	// addr
	mbg_rxTxBuffer[i++] = mf->addr;

	// function code
	mbg_rxTxBuffer[i++] = mf->code;

	// data
	uint32_t k;
	for (k = 0; k < mf->dataLen; k++, i++)
		mbg_rxTxBuffer[i] = mf->data[k];

	// caluclate crc
	uint16_t calcCrc = mbg_CalculateCrc(mbg_rxTxBuffer, i);

	// check match
	if (calcCrc != mf->crc)
		retVal = HAL_ERROR;

	return retVal;
}

/*
 * @brief	Initializes the common hardware part for MODBUS master/ slave.
 * 			Run it inside master/ slave init function.
 * @param	uHandle: Uart handle pointer.
 * @param	HAL_OK if initialization complete.
 */
HAL_StatusTypeDef mbg_UartInit(UART_HandleTypeDef* uHandle)
{
	assert_param(uHandle);
	if (uHandle == NULL)
		return HAL_ERROR;

	HAL_StatusTypeDef retVal = HAL_OK;

	/*
	 * TODO: Initialize the serial parameters here maybe?
	 */

	// Init send mutex
	osMutexDef_t tempMutDef;
	mbg_sendMut = osMutexCreate(&tempMutDef);
	if (!mbg_sendMut)
		retVal++;

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
		return HAL_OK;

	return HAL_ERROR;
}

/*
 * @brief	Sends the specified \ref mf through \ref uHandle.
 * 			Also calculates the CRC for the frame.
 * @param	uHandle: uart handler struct pointer.
 * @param	mf: frame to be sent pointer.
 * @return	HAL_OK if sent succesfull.
 */
HAL_StatusTypeDef mbg_SendFrame(UART_HandleTypeDef* uHandle, modbusFrame_t* mf)
{
	if (!uHandle)
		return HAL_ERROR;

	// start stuffing the buffer with data
	uint32_t i = 0;

	// address
	mbg_rxTxBuffer[i++] = mf->addr;

	// func code
	mbg_rxTxBuffer[i++] = mf->code;

	// data
	uint32_t k;
	for (k = 0; k < mf->dataLen; k++, i++)
		mbg_rxTxBuffer[i] = mf->data[k];

	// caluclate crc
	uint16_t calcCrc = mbg_CalculateCrc(mbg_rxTxBuffer, i);

	// add the crc to the buffer
	mbg_rxTxBuffer[i++] = (uint8_t)calcCrc;
	mbg_rxTxBuffer[i++] = (uint8_t)(calcCrc >> 8);

	// send the data and return
	return mbg_SendData(uHandle, mbg_rxTxBuffer, i);
}

/*
 * @brief	Sends specified \ref data of size \ref len, using \ref uHandle.
 * 			Data is transmitted in half duplex mode.
 * @param	uHandle: uart handle using which data will be sent.
 * @param	data: pointer to the data stream.
 * @param	len: nr of bytes to send.
 * @return	HAL_OK if transmission succesfull.
 */
HAL_StatusTypeDef mbg_SendData(UART_HandleTypeDef* uHandle, uint8_t* data, size_t len)
{
	if (!len)
		return HAL_OK; // nothing to send

	assert_param(uHandle);
	assert_param(data);
	HAL_StatusTypeDef retVal = HAL_OK;

	// lock
	osMutexWait(mbg_sendMut, osWaitForever);

	// send with DMA
	while (mbg_CheckSendStatus(uHandle))
		osDelay(1);

	retVal += HAL_UART_Transmit_DMA(uHandle, data, len);

	// release
	osMutexRelease(mbg_sendMut);
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
	if (!len)
		return HAL_OK; // nothing sent

	assert_param(uHandle);
	assert_param(data);
	HAL_StatusTypeDef retVal = HAL_OK;

	// Disable RX timeout interrupt
	mbg_DisableRxTimeout(uHandle);

	// Disable uart RX DMA
	retVal += HAL_UART_DMAStop(uHandle);

	// Wait for ready status
	while (HAL_UART_GetState(uHandle) != HAL_UART_STATE_READY);

	// Enable receive
	retVal += HAL_UART_Receive_DMA(uHandle, data, len);

	// Enable Rx timeout if rx DMA is ok and parameter is set
	if ((!retVal) && enableRxTimeout)
		mbg_EnableRxTimeout(uHandle);
	else
		mbg_DisableRxTimeout(uHandle);

	return retVal;
}

/*
 * @brief	Uart TX complete handle overwrite function for both master and slave modules.
 * 			Depending on which driver is used the proper function has to be
 * 			overwritten in master/ slave module.
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	__asm("NOP");
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
 * @brief	ModBus MASTER weak override function. Do NOT override it in SLAVE framework.
 * @param	uHandle: pointer to the uart modbus master struct.
 */
__attribute__((weak)) void mbs_uartRxRoutine(UART_HandleTypeDef* uHandle) { }

/*
 * @brief	ModBus SLAVE weak override function. Do NOT override it in MASTER framework.
 * @param	uHandle: pointer to the uart modbus slave struct.
 */
__attribute__((weak)) void mbm_uartRxRoutine(UART_HandleTypeDef* uHandle) { }

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




