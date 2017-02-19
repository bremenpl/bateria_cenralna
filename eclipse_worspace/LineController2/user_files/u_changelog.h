/*
 * changelog.h
 *
 *  Created on: 07.01.2016
 *      Author: Lukasz
 *
 *      Format: 32 bit variable represended as 3.2.1.0 (0x33221100)
 *
 *      0- Minor change, ie. existing feature bug fix
 *      1- Additive change, ie. new feature
 *      2- Major change, ie. New functionalities
 *      3- Very large change, ie. new MCU used etc.
 *
 *     CHANGELOG:
 *
 *     v1.0.0.0
 *     		- Initial release.
 *
 *
 *
 *
 */

#ifndef U_CHANGELOG_H_
#define U_CHANGELOG_H_

#include "u_types.h"

static const firmware_t firmwareNr =
{
	.firm8[0] = 0,
	.firm8[1] = 0,
	.firm8[2] = 0,
	.firm8[3] = 1
};

// New file template

/* Includes ------------------------------------------------------------------*/

/* Defines and macros --------------------------------------------------------*/

/* Enums and structs ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Public variables ----------------------------------------------------------*/

/* Fuction prototypes --------------------------------------------------------*/

/* Function declarations -----------------------------------------------------*/


#endif /* U_CHANGELOG_H_ */
