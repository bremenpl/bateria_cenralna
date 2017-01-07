/*
 * u_mbmHandler.c
 *
 *  Created on: 07.01.2017
 *      Author: Lukasz
 */

/* Includes ------------------------------------------------------------------*/
#include "u_mbmHandler.h"
#include "u_logger.h"

/* Defines and macros --------------------------------------------------------*/

/* Enums and structs ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
char lc_dataPrintBuf[MBG_MAX_DATA_LEN * 3]; // max data 2 signs + space
osMutexId lc_printMutex;

/* Public variables ----------------------------------------------------------*/
mbmUart_t mbmu;

/* Fuction prototypes --------------------------------------------------------*/

/* Function declarations -----------------------------------------------------*/

/*
 * @brief	Initializes the line controller module
 */
HAL_StatusTypeDef lcHandler_Init()
{
	HAL_StatusTypeDef retVal = HAL_OK;

	// create print mutex
	osMutexDef_t tempMutDef;
	lc_printMutex = osMutexCreate(&tempMutDef);
	assert_param(lc_printMutex);

	if (!lc_printMutex)
		retVal++;

	return retVal;
}

// overrides
/*
 * @brief	Read holding registers override function
 * @param	mf: pointer to a modbus frame struct.
 */
void mbs_CheckReadHoldingRegistersRequest(const mbgFrame_t* const mf)
{
	log_PushLine(e_logLevel_Info, "Read Holding registers response");
}

/*
 * @brief	ModBus GENERIC strong overrride function.
 * 			Use it to print request and response frames.
 * @param	mf: modbus frame to print.
 */
void mbg_uartPrintFrame(const mbgFrame_t* const mf)
{
	assert_param(mf);

	char txtType[] = "RESP";
	if (e_mbgMesgType_Request == mf->msgType)
		sprintf(txtType, "%s", "REQT");

	// lock printing
	osMutexWait(lc_printMutex, osWaitForever);

	for (uint32_t i = 0; i < mf->dataLen; i++)
		sprintf(lc_dataPrintBuf + (i * 3), "%02X ", mf->data[i]);

	// print
	log_PushLine(e_logLevel_Info, "%s: SADDR:0x%X FCODE:%u DLEN:%u DATA:%sCRC:0x%X",
			txtType, mf->addr, mf->code, mf->dataLen, lc_dataPrintBuf, mf->crc);

	// enable printing
	osMutexRelease(lc_printMutex);
}






