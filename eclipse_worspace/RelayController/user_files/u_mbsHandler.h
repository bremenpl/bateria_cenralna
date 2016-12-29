/*
 * u_mbsHandler.h
 *
 *  Created on: 28.12.2016
 *      Author: Lukasz
 */

#ifndef U_MBSHANDLER_H_
#define U_MBSHANDLER_H_


/* Includes ------------------------------------------------------------------*/
#include "u_modBusSlave.h"

/* Defines and macros --------------------------------------------------------*/
#define		RC_NO_OF_REG		128
#define 	RC_NO_OF_COILS		(RC_NO_OF_REG * 8)

/* Enums and structs ---------------------------------------------------------*/

/*
 * @brief	Relay controller register map
 */
typedef struct
{
	uint16_t	reg[RC_NO_OF_REG];
	uint16_t	access[RC_NO_OF_REG];

} rcRegMap_t;

typedef enum
{
	e_rcAccessType_RW					= 0,
	e_rcAccessType_R					= 1,
	e_rcAccessType_W					= 2,

} rcAccessType_t;

typedef enum
{
	e_rcStatusMask_RelayStateR			= 1,
	e_rcStatusMask_PowerSourceR			= 2,
	e_rcStatusMask_BlackoutBehaviourR	= 4,

} rcStatusMaskR_t;

typedef enum
{
	e_rcStatusMask_RelayStateW			= 1,
	e_rcStatusMask_BlackoutBehaviourW	= 2,

} rcStatusMaskW_t;

/* Private variables ---------------------------------------------------------*/

/* Public variables ----------------------------------------------------------*/

/* Fuction prototypes --------------------------------------------------------*/
void mbsHandler_Init();

/* Function declarations -----------------------------------------------------*/



#endif /* U_MBSHANDLER_H_ */
