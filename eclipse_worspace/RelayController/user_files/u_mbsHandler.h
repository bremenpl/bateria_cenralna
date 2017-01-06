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

#define 	REL_LED_GPIO		GPIOA
#define 	REL_LED_PIN			GPIO_PIN_5

/* Enums and structs ---------------------------------------------------------*/

/*
 * @brief	Relay controller register map
 */
typedef struct
{
	uint16_t	reg[RC_NO_OF_REG];
	uint16_t	access[RC_NO_OF_REG];

} rcRegSpace_t;

typedef enum
{
	e_rcRegMap_Status					= 0x00,
	e_rcRegMap_Setting					= 0x01,
	e_rcRegMap_IdFirst					= 0x02,
	e_rcRegMap_IdLast					= 0x07,
	e_rcRegMap_StringFirst				= 0x08,
	e_rcRegMap_StringLast				= 0x28,

} rcRegMap_t;

typedef enum
{
	e_rcAccessType_RW					= 0,
	e_rcAccessType_R					= 1,
	e_rcAccessType_W					= 2,

} rcAccessType_t;

typedef enum
{
	e_rcStatusMask_RelayStateR			= 0,
	e_rcStatusMask_PowerSourceR			= 1,
	e_rcStatusMask_BlackoutBehaviorR	= 2,

} rcStatusMaskR_t;

typedef enum
{
	e_rcStatusMask_RelayStateW			= 0,
	e_rcStatusMask_BlackoutBehaviorW	= 1,

} rcStatusMaskW_t;

/* Private variables ---------------------------------------------------------*/

/* Public variables ----------------------------------------------------------*/

/* Fuction prototypes --------------------------------------------------------*/
void mbsHandler_Init();

/* Function declarations -----------------------------------------------------*/



#endif /* U_MBSHANDLER_H_ */
