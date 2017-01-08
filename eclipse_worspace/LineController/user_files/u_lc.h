/*
 * u_lc.h
 *
 *  Created on: 07.01.2017
 *      Author: Lukasz
 */

#ifndef U_LC_H_
#define U_LC_H_


/* Includes ------------------------------------------------------------------*/
#include "u_modBusMaster.h"
#include "u_modBusSlave.h"

/* Defines and macros --------------------------------------------------------*/
#define LC_MAX_PRINT_CHARS		90

#define LC_NO_OF_MODULES		1
#define LC_NO_OF_RCS			20

#define LC_PING_PERIOD			100
#define LC_PING_TIMEOUT			50
#define LC_RESP_QUEUE_TIMEOUT	500
#define LC_PINGS_TILL_PRESENT	3

#define LC_PINGLED_GPIO			GPIOA
#define LC_PINGLED_PIN			(1 << 5)

/* Enums and structs ---------------------------------------------------------*/

typedef enum
{
	e_rcRegMap_Status					= 0x00,
	e_rcRegMap_Setting					= 0x01,
	e_rcRegMap_IdFirst					= 0x02,
	e_rcRegMap_IdLast					= 0x07,
	e_rcRegMap_StringFirst				= 0x08,
	e_rcRegMap_StringLast				= 0x28,

} rcRegMap_t;

/*
 * @brief	Single relay controller struct
 */
typedef struct
{
	uint8_t			presence;			/*!< non zero indicates present device */
	uint8_t			pings;				/*!< Defines presence */

	uint16_t		status;				/*!< Status register */
	uint8_t			addr;				/*!< Slave address */

} rc_t;

/*
 * @brief	This structure provides all relevant information needed for indentification
 * 			of the arrived slave response to the master.
 */
typedef struct
{
	const mbmUart_t*	mbm;			/*!< Master module for which the response came */
	mbgFrame_t			rcRespFrame;	/*!< Relay controller response frame */

} mRespPack_t;

/*
 * @brief	Master and slave modules from lc struct are connected in one chain
 */
typedef struct
{
	mbmUart_t		mbm;				/*!< mosbus master module */
	mbsUart_t		mbs;				/*!< mosbus slave module */

	osThreadId 		sysId_Ping;			/*!< Pinging thread ID */
	osMessageQId 	msgQId_Ping;		/*!< Msg queue for ack */
	osMutexId		rcAccesMut;			/*!< Relay controller access mutex */
	mRespPack_t		rcResp;				/*!< Relay controller response buffer (1 item) */

	rc_t			rc[LC_NO_OF_RCS];	/*!< Relay controllers */


} lc_t;


/* Private variables ---------------------------------------------------------*/

/* Public variables ----------------------------------------------------------*/
extern lc_t	lc[LC_NO_OF_MODULES];

/* Fuction prototypes --------------------------------------------------------*/

/* Function declarations -----------------------------------------------------*/
HAL_StatusTypeDef lc_Init();


#endif /* U_LC_H_ */
