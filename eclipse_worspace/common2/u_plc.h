/*
 * u_plcMaster.h
 *
 *  Created on: 07.04.2017
 *      Author: Lukasz
 */

#ifndef U_PLC_H_
#define U_PLC_H_

/* Includes ------------------------------------------------------------------*/

#include "stm32f0xx_hal.h"
#include "cmsis_os.h"

/* Defines and macros --------------------------------------------------------*/
#define PLC_MAX_MODULES			4
#define	PLC_EXP_TIL_DC			6

/* Enums and structs ---------------------------------------------------------*/

typedef enum
{
	e_plcDevType_Slave		= 0,
	e_plcDevType_Master		= 1,
} plcDevType_t;

/*
 * @brief	PLC master behavior
 */
typedef enum
{
	e_plcMasBeh_UseZc		= 0,			/*!< Transmits only when AC present */
	e_plcMasBeh_UseNoZc,					/*!< Transmits only when DC present */
	e_plcMasBeh_UseZcAndNoZc,				/*!< Transmits in both */
} plcMasBeh_t;

typedef struct
{
	uint32_t			period;
	uint32_t			prescaler;
} timParams_t;

typedef enum
{
	e_plcState_Idle					= 0,

	e_plcState_SendWaitForNextZc,
	e_plcState_SendWaitForSyncBitDone,
	e_plcState_SendWaitForDataBitDone,
	e_plcState_SendWaitForNextSyncTimeout,

	e_plcState_RecvWaitForSyncBit1st,
	e_plcState_RecvWaitForSyncBit,
	e_plcState_RecvWaitForSyncBitTimeout,

} plcState_t;

typedef enum
{
	e_plcSupplyType_Unspecified		= 0,
	e_plcSupplyType_Ac				= 1,
	e_plcSupplyType_Dc				= 2,

} plcSupplyType_t;

typedef enum
{
	e_plcTransStatus_Idle			= 0,
	e_plcTransStatus_Sending		= 1,
	e_plcTransStatus_Receiving		= 2,
} plcTransStatus_t;

typedef struct
{
	uint8_t*			data;				/*!< pointer to the data array */
	uint32_t			len;				/*!< data length */

	uint32_t			bitIndex;			/*!< Current bit to send */
	uint32_t			byteIndex;			/*!< Current byte to send */
	plcTransStatus_t	status;				/*!< Current module status */

} plcTransStruct_t;

typedef struct
{
	// preset params
	GPIO_TypeDef* 		zCgpio;				/*!< General Purpose I/O 		*/
	uint16_t 			zCpin;				/*!< GPIO_pins type 			*/
	uint32_t 			strobe;				/*!< Non zero equals rising 	*/

	GPIO_TypeDef* 		plcIngpio;			/*!< General Purpose I/O 		*/
	uint16_t 			plcInpin;			/*!< GPIO_pins type 			*/

	GPIO_TypeDef* 		plcOutgpio;			/*!< General Purpose I/O 		*/
	uint16_t 			plcOutpin;			/*!< GPIO_pins type 			*/

	TIM_HandleTypeDef*	timHandleSync;		/*!< Used for ZC sync 			*/
	TIM_HandleTypeDef*	timHandleBits;		/*!< Used for bit timing 		*/
	TIM_HandleTypeDef*	timHandleZc;				/*!< Used for checking AC/DC	*/

	plcDevType_t		devType;			/*!< Slave or Master 			*/
	plcMasBeh_t			devBeh;				/*!< Behavior on AC and DC		*/

	// calculated params

	/*
	 * Bits timer is is used for measuring how long should the bit last in send mode
	 * In receive mode at timer expiration the received bit state is checked.
	 */
	timParams_t			timParamsBitsSend;	/*!< trasmit bits timer settings */
	timParams_t			timParamsBitsRecv;	/*!< Receive bits timer settings */

	/*
	 * In send mode at its expiration next sync + bit is sent.
	 * In receive or idle mode, its used for calculating next chunck timeout
	 * or AC failure accrdongly.
	 */
	timParams_t			timParamsSyncSend;	/*!< trasmit bits timer settings */
	timParams_t			timParamsSyncRecv;	/*!< Receive bits timer settings */

	// operational variables
	plcState_t			state;				/*!< Module machine state 		*/
	plcSupplyType_t		supply;				/*!< Ac voltage present 		*/
	plcTransStruct_t	trans;				/*!< Transmission struct 		*/
	uint8_t				recvByte;			/*!< Received bytes pre buffer 	*/
	uint32_t			noZcCounter;		/*!< incremented in ZC timer	*/

} plcm_t;

/* Private variables ---------------------------------------------------------*/

/* Public variables ----------------------------------------------------------*/

/* Fuction prototypes --------------------------------------------------------*/

/* Function declarations -----------------------------------------------------*/

HAL_StatusTypeDef plcm_SetTimerFreq(const float freq, TIM_HandleTypeDef* timer);
HAL_StatusTypeDef plcm_Init(plcm_t* p, uint32_t* id);

void plcm_SetOutState(const uint32_t id, const uint32_t state);
void plcm_TogOutState(const uint32_t id);
uint32_t plcm_GetInState(const uint32_t id);
uint32_t plcm_GetZcState(const uint32_t id);
void plcm_timHandle(TIM_HandleTypeDef* timer);
void plcm_extiHandle(uint16_t GPIO_Pin);
void plcm_StartSendAtNextSync(plcm_t* p);
HAL_StatusTypeDef plcm_GetIdFromPlc(plcm_t* p, uint32_t* id);

void plcm_BitsTimerHandleSlave(const uint32_t id);
void plcm_BitsTimerHandleMaster(const uint32_t id);
void plcm_SyncTimerHandleSlave(const uint32_t id);
void plcm_SyncTimerHandleMaster(const uint32_t id);
void plcm_ZcTimerHandleMaster(const uint32_t id);
void plcm_InPinHandleSlave(const uint32_t id);
void plcm_InPinHandleMaster(const uint32_t id);
void plcm_resetTransData(plcm_t* plc);

// overrides
void plcm_txRoutine(plcm_t* plcHandle);
void plcm_rxRoutine(plcm_t* plcHandle);




#endif /* U_PLC_H_ */
