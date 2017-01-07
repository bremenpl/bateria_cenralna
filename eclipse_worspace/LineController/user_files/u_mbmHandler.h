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

/* Defines and macros --------------------------------------------------------*/

/* Enums and structs ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Public variables ----------------------------------------------------------*/
extern mbmUart_t mbmu;

/* Fuction prototypes --------------------------------------------------------*/

/* Function declarations -----------------------------------------------------*/
HAL_StatusTypeDef lcHandler_Init();


#endif /* U_MBMHANDLER_H_ */
