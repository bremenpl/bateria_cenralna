/*
 * u_mbmHandler.h
 *
 *  Created on: 07.01.2017
 *      Author: Lukasz
 */

#ifndef U_MBMHANDLER_H_
#define U_MBMHANDLER_H_


/* Includes ------------------------------------------------------------------*/
#include "u_modBusMaster.h"
#include "u_modBusSlave.h"

/* Defines and macros --------------------------------------------------------*/
#define LC_MAX_PRINT_CHARS		90

/* Enums and structs ---------------------------------------------------------*/

/*
 * @brief	Master and slave modules from lc struct are connected in one chain
 */
typedef struct
{
	mbmUart_t	mbm;					/*!< mosbus master module */
	mbsUart_t	mbs;					/*!< mosbus slave module */

} lc_t;

/* Private variables ---------------------------------------------------------*/

/* Public variables ----------------------------------------------------------*/
extern lc_t	lc;

/* Fuction prototypes --------------------------------------------------------*/

/* Function declarations -----------------------------------------------------*/
HAL_StatusTypeDef lcHandler_Init();


#endif /* U_MBMHANDLER_H_ */
