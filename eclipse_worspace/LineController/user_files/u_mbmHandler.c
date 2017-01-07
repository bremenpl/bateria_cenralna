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

/* Public variables ----------------------------------------------------------*/
lc_t	lc;

/* Fuction prototypes --------------------------------------------------------*/

/* Function declarations -----------------------------------------------------*/

/*
 * @brief	Initializes the line controller module
 */
HAL_StatusTypeDef lcHandler_Init()
{
	HAL_StatusTypeDef retVal = HAL_OK;

	// TODO add ping thread

	return retVal;
}

// overrides
/*
 * @brief	Read holding registers override function
 * @param	mf: pointer to a modbus frame struct.
 */
void mbm_CheckReadHoldingRegisters(const mbgFrame_t* const mf)
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
	char databuf[LC_MAX_PRINT_CHARS];

	char txtType[] = "RESP";
	if (e_mbgMesgType_Request == mf->msgType)
		sprintf(txtType, "%s", "REQT");

	if ((mf->dataLen * 3) <= (LC_MAX_PRINT_CHARS - 6))
	{
		for (uint32_t i = 0; i < mf->dataLen; i++)
			sprintf(databuf + (i * 3), "%02X ", mf->data[i]);
	}
	else
	{
		uint32_t i;
		for (i = 0; i < ((LC_MAX_PRINT_CHARS / 3) - 2); i++)
			sprintf(databuf + (i * 3), "%02X ", mf->data[i]);

		// append dots so user reading the log will know its not the full msg
		sprintf(databuf + (i * 3),"%s", "... ");
	}

	// print
	log_PushLine(e_logLevel_Info, "%s: SADDR:0x%X FCODE:%u DLEN:%u DATA:%sCRC:0x%X",
			txtType, mf->addr, mf->code, mf->dataLen, databuf, mf->crc);
}






