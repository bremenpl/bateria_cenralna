/*
 * u_lc.c
 *
 *  Created on: 07.01.2017
 *      Author: Lukasz
 */

/* Includes ------------------------------------------------------------------*/
#include "u_lc.h"
#include "u_logger.h"

/* Defines and macros --------------------------------------------------------*/

/* Enums and structs ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Public variables ----------------------------------------------------------*/
lc_t	lc[LC_NO_OF_MODULES];

/* Fuction prototypes --------------------------------------------------------*/
HAL_StatusTypeDef lc_pingDevice(lc_t* l, uint32_t addr, uint16_t* status);
void lc_managePresence(rc_t* rc, HAL_StatusTypeDef respStatus);
void lc_manageRcPresBits(lc_t* l, const uint32_t rcNr);

void lc_pingTask(void const* argument);

/* Function declarations -----------------------------------------------------*/

/*
 * @brief	Initializes the line controller module
 */
HAL_StatusTypeDef lc_Init()
{
	HAL_StatusTypeDef retVal = HAL_OK;

	// create RC response queue
	osMessageQDef_t msgDef_temp;
	msgDef_temp.item_sz = sizeof(mRespPack_t*);
	msgDef_temp.queue_sz = 1; // In a properly working modbus system 1 is enough

	// declare ping thread
	osThreadDef(pingTask, lc_pingTask, osPriorityNormal, LC_NO_OF_MODULES, 512);

	// do everything for each module
	for (size_t i = 0; i < LC_NO_OF_MODULES; i++)
	{
		// create the queue
		lc[i].msgQId_Ping = osMessageCreate(&msgDef_temp, NULL);
		if (!lc[i].msgQId_Ping)
			retVal++;

		// create thread for dequeuing
		lc[i].sysId_Ping = osThreadCreate(osThread(pingTask), lc + i);
		if (!lc[i].sysId_Ping)
			retVal++;

		// assign addresses of RC, they are needed further in the code
		for (uint32_t k = 0; k < LC_NO_OF_RCS; k++)
			lc[i].rc[k].addr = k + 1;

		// get unique ID
		mbg_getUniqId(lc[i].uniqId);
	}

	return retVal;
}

/*
 * @brief	Pinging thread
 * @param	pointer to the lc struct
 */
void lc_pingTask(void const* argument)
{
	assert_param(argument);

	// get the pointer
	lc_t* l = (lc_t*)argument;
	uint32_t i, addr;
	portTickType xLastWakeTime = xTaskGetTickCount();

	while (1)
	{
		// ping loop
		for (i = 0, addr = 1; i < LC_NO_OF_RCS; i++, addr++)
		{
			//log_PushLine(e_logLevel_Debug, "Ping ID= %02u", i);

			// try to ping next slave
			lc_managePresence(&l->rc[i] ,lc_pingDevice(l, addr, &l->rc[i].status));

			// maintain presence table for the master PC
			lc_manageRcPresBits(l, i);

			// so the intervals between pings are const
			vTaskDelayUntil( &xLastWakeTime, (LC_PING_PERIOD / portTICK_RATE_MS));
		}
	}
}

/*
 * @brief	Pings the provided \ref addr device. If ping is succesfull,
 * 			HAL_OK is returned and proper value stored under \ref status.
 * @param	l: Line controller module
 * @param	addr: Relay controller modbus slave address
 * @param	status: pointer to variable under which status register content is saved
 * @return	HAL_OK if ping was successful.
 */
HAL_StatusTypeDef lc_pingDevice(lc_t* l, uint32_t addr, uint16_t* status)
{
	// cannot ping addr 0
	if (!addr)
		return HAL_ERROR;

	assert_param(status);
	assert_param(l);
	osEvent retEvent;
	mRespPack_t* resp;

	// read the status register
	if (mbm_RequestReadHoldingRegs(&l->mbm, addr, e_rcRegMap_Status, 1))
		return HAL_ERROR;

	// if frame was sent successful (or rather is being sent through DMA right now),
	// await for the response in here for the timeout time
	retEvent = osMessagePeek(l->msgQId_Ping, LC_PING_TIMEOUT);
	HAL_GPIO_WritePin(LC_ERROR_GPIO, LC_ERROR_PIN, 0);

	if (osEventMessage == retEvent.status)
	{
		resp = (mRespPack_t*)retEvent.value.p;

		// check if this is proper master
		if (&l->mbm != resp->mbm)
			return HAL_ERROR;

		// debug
		HAL_GPIO_WritePin(LC_ERROR_GPIO, LC_ERROR_PIN, 1);

		// save status
		*status = (resp->rcRespFrame.data[1] << 8) | (resp->rcRespFrame.data[2] >> 8);

		// remove the message
		osMessageGet(l->msgQId_Ping, LC_PING_TIMEOUT);
		return HAL_OK;
	}

	return HAL_ERROR;
}

/*
 * @brief	Manages the ping system for \ref rc device.
 * @param	rc: pointer to rc device.
 * @param	respStatus: Response from ping.
 */
void lc_managePresence(rc_t* rc, HAL_StatusTypeDef respStatus)
{
	assert_param(rc);

	if (HAL_OK == respStatus)
	{
		// increment pings if possible
		if (rc->pings >= (LC_PINGS_TILL_PRESENT - 1))
		{
			if (!rc->presence) // present
			{
				rc->pings++;
				rc->presence = 1;
				log_PushLine(e_logLevel_Info, "Slave 0x%02X present", rc->addr);
			}
		}
		else // not present yet
		{
			rc->pings++;
			log_PushLine(e_logLevel_Debug,
					"Slave 0x%02X gaining presence (%u)", rc->addr, rc->pings);
		}
	}
	else
	{
		if (rc->pings <= 1)
		{
			if (rc->presence) // just lost presence
			{
				rc->pings--;
				rc->presence = 0;
				log_PushLine(e_logLevel_Info, "Slave 0x%02X absent", rc->addr);
			}
		}
		else // loosing it
		{
			rc->pings--;
			log_PushLine(e_logLevel_Debug,
					"Slave 0x%02X loosing presence (%u)", rc->addr, rc->pings);
		}
	}
}

/*
 * @brief	Sets or clears the presence bits (they start at address 0) of the relay
 * 			controllers table. This is used for the master PC to check either RC's
 * 			are present or not.
 * @param	l: Line controller module
 * @param	rcNr: rc to manage
 */
void lc_manageRcPresBits(lc_t* l, const uint32_t rcNr)
{
	assert_param(l);
	if (rcNr >= LC_NO_OF_RCS)
		return;

	uint32_t byte = rcNr / 8;
	uint32_t bit = rcNr % 8;

	// check presence
	if (l->rc[rcNr].presence)
		l->rcPresBits[byte] |=  (1 << bit);
	else
		l->rcPresBits[byte] &= ~(1 << bit);
}

// overrides
/*
 * @brief	Read holding registers override function
 * @param	mbm: master module pointer.
 * @param	mf: pointer to a modbus frame struct.
 */
void mbm_CheckReadHoldingRegisters(const mbmUart_t* const mbm, const mbgFrame_t* const mf)
{
	assert_param(mbm);
	assert_param(mf);

	//log_PushLine(e_logLevel_Info, "Read Holding registers response");

	// enqueue the response, find the master
	for (uint32_t i = 0; i < LC_NO_OF_MODULES; i++)
	{
		if (mbm == &lc[i].mbm)
		{
			// copy data
			lc[i].rcResp.mbm = mbm;
			lc[i].rcResp.rcRespFrame = *mf;

			if (osOK != osMessagePut(lc[i].msgQId_Ping,
					(uint32_t)&lc[i].rcResp, LC_RESP_QUEUE_TIMEOUT))
				log_PushLine(e_logLevel_Warning, "Unable to enque the response");
		}
	}
}

mbgExCode_t mbs_CheckReadHoldingRegisters(const mbsUart_t* const mbs, mbgFrame_t* mf)
{
	assert_param(mf);
	mbgExCode_t retVal = e_mbsExCode_illegalDataAddr;
	uint32_t k;
	bool found = false;

	// find object
	for (k = 0; k < LC_NO_OF_MODULES; k++)
	{
		if (mbs == &lc[k].mbs)
		{
			found = true;
			break;
		}
	}

	if (!found)
		return retVal;

	// start address
	uint16_t startAddr = mf->data[1] | ((uint16_t)mf->data[0] << 8);

	// quantity of registers
	uint16_t nrOfRegs = mf->data[3] | ((uint16_t)mf->data[2] << 8);

	// byte count
	mf->data[0] = nrOfRegs * 2; // nr of registers * 2 (bytes)
	// update frame data length member
	mf->dataLen = mf->data[0] + 1; // 1 for byte count

	switch (startAddr)
	{
		case e_rcRegMap_Status: // fixed presence status
		{
			// check if registers to read are in range
			if ((startAddr + nrOfRegs) > LC_NO_OF_PRES_REGS)
				return e_mbsExCode_illegalDataAddr;

			// register values
			for (uint32_t addr = 0, i = 1; i < (mf->data[0] + 1); i += 2, addr++)
			{
				mf->data[i] = (uint8_t)(lc[k].rcPresBits[addr] >> 8); 	// HI
				mf->data[i + 1] = (uint8_t)lc[k].rcPresBits[addr]; 		// LO
			}

			retVal = e_mbsExCode_noError;
			break;
		}

		case e_rcRegMap_IdFirst:
		{
			// check if registers to read are in range
			if (nrOfRegs > (e_rcRegMap_IdLast - startAddr + 1))
				return e_mbsExCode_illegalDataAddr;

			// register values
			for (uint32_t addr = 0, i = 1; i < (mf->data[0] + 1); i += 2, addr++)
			{
				mf->data[i] = (uint8_t)(lc[k].uniqId[addr] >> 8); 	// HI
				mf->data[i + 1] = (uint8_t)lc[k].uniqId[addr]; 		// LO
			}

			retVal = e_mbsExCode_noError;
			break;
		}

		default: retVal = e_mbsExCode_illegalDataAddr;
	}

	return retVal;
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
	log_PushLine(e_logLevel_Debug, "%s: ADDR:0x%02X CODE:%02u DLEN:%02u DATA:%sCRC:0x%04X",
			txtType, mf->addr, mf->code, mf->dataLen, databuf, mf->crc);
}






