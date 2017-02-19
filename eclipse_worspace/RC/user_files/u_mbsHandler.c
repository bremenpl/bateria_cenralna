/*
 * u_mbsHandler.c
 *
 *  Created on: 28.12.2016
 *      Author: Lukasz
 */


/* Includes ------------------------------------------------------------------*/
#include <string.h>

#include "u_mbsHandler.h"

/* Defines and macros --------------------------------------------------------*/

/* Enums and structs ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static rcRegSpace_t rcRegMap;
static uint16_t* rcStatus;
static uint16_t* rcSetting;
static uint16_t* rcUniqId;
static uint16_t* rcUserString;

/* Public variables ----------------------------------------------------------*/

/* Fuction prototypes --------------------------------------------------------*/
mbgExCode_t rc_SetRegisterBit(const uint16_t coil, const uint16_t value);
void rc_UpdateRegChanges(const uint16_t reg);

/* Function declarations -----------------------------------------------------*/

/*
 * Initializes modbus rcv handler
 */
void mbsHandler_Init()
{
	// populate register map according to the register map document
	// also assign apropriate pointers
	// TODO: add reading from eeprom

	// Register	Address	Access	Description
	// Status	0x00	R		Status bits reflects the current state of the device.
	//							Those registers cannot be written but can be read at any time.
	rcRegMap.access[e_rcRegMap_Status] = e_rcAccessType_R;
	rcStatus = rcRegMap.reg;

	// Setting	0x01	W		One can write those bits in order to program the device.
	// 							Although this register is read-write accessed,
	//							its state when read does not reflect the current state,
	// 							but the value that was written to it. In order to obtain
	//							current device state Status register should be read.
	rcRegMap.access[e_rcRegMap_Setting] = e_rcAccessType_W;
	rcSetting = rcRegMap.reg + e_rcRegMap_Setting;

	// ID	0x02 - 0x07	R		96 bit (12 byte) unique device ID
	// Apparently not all F0 devices own a unique device ID...
	// https://community.st.com/thread/28751
	// In this case one has to be assigned in the eeprom by the user
	for (uint32_t i = e_rcRegMap_IdFirst; i <= e_rcRegMap_IdLast; i++)
	{
		rcRegMap.access[i] = e_rcAccessType_R;
		rcRegMap.reg[i] = i; // TODO temp
	}

	rcUniqId = rcRegMap.reg + e_rcRegMap_IdFirst;

	// String	0x08 - 0x28	RW	64 byte user string.
	//							One can use it to create a unique name for the device.
	char* tempString = "RelayController";
	memcpy(rcRegMap.reg + e_rcRegMap_StringFirst, tempString, 16); // TODO temp

	for (uint32_t i = e_rcRegMap_StringFirst; i <= e_rcRegMap_StringLast; i++)
		rcRegMap.access[i] = e_rcAccessType_RW;

	rcUserString = rcRegMap.reg + e_rcRegMap_StringFirst;
}

/*
 * @brief	Sets the state (\ref value) of the \ref coil.
 * @param	coil: coil number/ address
 * @param	value: non zero sets the bit, zero clears it.
 * @param	returns \ref e_mbsExCode_noError if operation was succesfull,
 * 			\ref e_mbsExCode_illegalDataValue if register is read only.
 */
mbgExCode_t rc_SetRegisterBit(const uint16_t coil, const uint16_t value)
{
	// validate coil address
	if (coil >= RC_NO_OF_COILS)
		return e_mbsExCode_illegalDataAddr;

	// validate coil value (0 for coil off, 0xFF00 for coil on)
	if ((value != 0) && (value != 0xFF00))
		return e_mbsExCode_illegalDataValue;

	// get the register in which the coil resides
	uint16_t reg = coil / 8;

	// check if register is not read-only
	if (e_rcAccessType_R == rcRegMap.access[reg])
		return e_mbsExCode_slaveDeviceFailure;

	// get the bit in the reg
	uint32_t bit = coil % 8;

	// set/ clear the bit
	if (value)
		rcRegMap.reg[reg] |= 1 << bit;
	else
		rcRegMap.reg[reg] &= ~(1 << bit);

	// trigger register dependent actions
	rc_UpdateRegChanges(reg);

	return e_mbsExCode_noError;
}

/*
 * @brief	Run this function when an action needs to be triggered after \ref reg
 * 			is modified. This function is device dependent.
 * @param	reg: register address that was updated and should trigger an action.
 */
void rc_UpdateRegChanges(const uint16_t reg)
{
	switch (reg)
	{
		case e_rcRegMap_Setting:
		{
			// When setting register is changed, status reg has to be updated
			// check all bytes

			// relay state
			if (rcRegMap.reg[reg] & (1 << e_rcStatusMask_RelayStateW))
			{
				rcRegMap.reg[e_rcRegMap_Status] |= 1 << e_rcStatusMask_RelayStateR;
				HAL_GPIO_WritePin(REL_LED_GPIO, REL_LED_PIN, 1);
			}
			else
			{
				rcRegMap.reg[e_rcRegMap_Status] &= ~(1 << e_rcStatusMask_RelayStateR);
				HAL_GPIO_WritePin(REL_LED_GPIO, REL_LED_PIN, 0);
			}

			// TODO: change the state of that relay

			// blackout behavior
			if (rcRegMap.reg[reg] & (1 << e_rcStatusMask_BlackoutBehaviorW))
				rcRegMap.reg[e_rcRegMap_Status] |= 1 << e_rcStatusMask_BlackoutBehaviorR;
			else
				rcRegMap.reg[e_rcRegMap_Status] &= ~(1 << e_rcStatusMask_BlackoutBehaviorR);

			// TODO: change current behavior if?

			break;
		}

		default: { } // some registers changes dont trigger anything
	}
}

// OVERRIDES

/*
 * @brief	Override function for handling incoming modbus requests
 * 			from the master device.
 */
mbgExCode_t mbs_CheckReadHoldingRegisters(const mbsUart_t* const mbs, mbgFrame_t* mf)
{
	(void)mbs; // only one module no need to check this
	if (!mf) return e_mbsExCode_illegalDataAddr;

	// start address
	uint16_t startAddr = mf->data[1] | ((uint16_t)mf->data[0] << 8);

	// quantity of registers
	uint16_t nrOfRegs = mf->data[3] | ((uint16_t)mf->data[2] << 8);

	// check if registers to read are in range
	if ((startAddr + nrOfRegs) > RC_NO_OF_REG)
		return e_mbsExCode_illegalDataAddr;

	// byte count
	mf->data[0] = nrOfRegs * 2; // nr of registers * 2 (bytes)

	// total len
	mf->dataLen = 1 + (uint16_t)mf->data[0]; // byte count + nr of registers * 2 (bytes)

	// register values
	for (uint32_t addr = startAddr, i = 1; i < (mf->data[0] + 1); i += 2, addr++)
	{
		mf->data[i] = (uint8_t)(rcRegMap.reg[addr] >> 8); 	// HI
		mf->data[i + 1] = (uint8_t)rcRegMap.reg[addr]; 		// LO
	}

	// return no error
	return e_mbsExCode_noError;
}

/*
 * @brief	Override function for handling incoming modbus requests
 * 			from the master device.
 */
mbgExCode_t mbs_CheckReadCoils(mbgFrame_t* mf)
{
	if (!mf) return e_mbsExCode_illegalDataAddr;

	// starting coild
	uint16_t startCoil = mf->data[1] | ((uint16_t)mf->data[0] << 8);

	// quantity of coils
	uint16_t nrOfCoils = mf->data[3] | ((uint16_t)mf->data[2] << 8);

	// check if registers to read are in range
	if ((startCoil + nrOfCoils) > RC_NO_OF_COILS)
		return e_mbsExCode_illegalDataAddr;

	// byte count
	mf->data[0] = (nrOfCoils / 8);

	// check residue
	if (nrOfCoils % 8)
		mf->data[0]++; // nr of coils / 8 + 1 if modulo not 0

	// total len
	mf->dataLen = 1 + (uint16_t)mf->data[0]; // byte count + coil status (bytes)

	// coil status
	// increment pointer byte wise not word wise
	uint8_t* curByte = ((uint8_t*)rcRegMap.reg) + (startCoil / 8);
	uint32_t curBit = startCoil % 8;

	// clear data initially
	memset(mf->data + 1, 0, mf->data[0]);

	// for loops bit per bit
	for (uint32_t coil = 0, byteInd = 1, totCoil = 0;
			totCoil < nrOfCoils;
			coil++, curBit++, totCoil++)
	{
		// check if bits need flipping
		if (coil >= 8)
		{
			coil = 0;
			byteInd++;
		}

		if (curBit >= 8)
		{
			curBit = 0;
			curByte++;
		}

		// check if selected bit is set
		if (*curByte & (1 << curBit))
			mf->data[byteInd] |= 1 << coil;
	}

	// clear residue
	//mf->data[mf->dataLen - 1] &= (0xFF >> (8 - (nrOfCoils % 8)));

	// TODO: check response bytes order

	// return no error
	return e_mbsExCode_noError;
}

/*
 * @brief	Override function for handling incoming modbus requests
 * 			from the master device.
 */
mbgExCode_t mbs_CheckWriteSingleCoil(mbgFrame_t* mf)
{
	if (!mf) return e_mbsExCode_illegalDataAddr;

	// output address
	uint16_t outAddr = mf->data[1] | ((uint16_t)mf->data[0] << 8);

	// output value
	uint16_t outVal = mf->data[3] | ((uint16_t)mf->data[2] << 8);

	// No need to update the mf for the response, as its a mirror of the request

	return rc_SetRegisterBit(outAddr, outVal);
}






