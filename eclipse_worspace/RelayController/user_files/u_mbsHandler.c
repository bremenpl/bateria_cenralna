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
static rcRegMap_t rcRegMap;
static uint16_t* rcStatus;
static uint16_t* rcSetting;
static uint16_t* rcUniqId;
static uint16_t* rcUserString;

/* Public variables ----------------------------------------------------------*/

/* Fuction prototypes --------------------------------------------------------*/

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
	rcRegMap.access[0] = e_rcAccessType_R;
	rcStatus = rcRegMap.reg;

	// Setting	0x01	W		One can write those bits in order to program the device.
	// 							Although this register is read-write accessed,
	//							its state when read does not reflect the current state,
	// 							but the value that was written to it. In order to obtain
	//							current device state Status register should be read.
	rcRegMap.access[1] = e_rcAccessType_W;
	rcSetting = rcRegMap.reg + 1;

	// ID	0x02 - 0x07	R		96 bit (12 byte) unique device ID
	// Apparently not all F0 devices own a unique device ID...
	// https://community.st.com/thread/28751
	// In this case one has to be assigned in the eeprom by the user
	for (uint32_t i = 0x02; i <= 0x07; i++)
	{
		rcRegMap.access[i] = e_rcAccessType_R;
		rcRegMap.reg[i] = i; // TODO temp
	}

	rcUniqId = rcRegMap.reg + 2;

	// String	0x08 - 0x28	RW	64 byte user string.
	//							One can use it to create a unique name for the device.
	char* tempString = "RelayController";
	memcpy(rcRegMap.reg + 8, tempString, 16); // TODO temp

	for (uint32_t i = 0x08; i <= 0x28; i++)
		rcRegMap.access[i] = e_rcAccessType_RW;

	rcUserString = rcRegMap.reg + 8;
}

/*
 * @brief	Override function for handling incoming modbus requests
 * 			from the master device.
 */
mbgExCode_t mbs_CheckReadHoldingRegistersRequest(mbgFrame_t* mf)
{
	if (!mf)
		return e_mbsExCode_illegalDataAddr;

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






