/*
 * u_types.h
 *
 *  Created on: 13 cze 2016
 *      Author: Lukasz
 */

#ifndef U_TYPES_H_
#define U_TYPES_H_


/* Includes ------------------------------------------------------------------*/

/* Defines and macros --------------------------------------------------------*/
#if defined(STM32F103xB) || defined(STM32F105xC)
	#include "stm32f1xx_hal.h"
#elif defined(STM32F091xC) || defined(STM32F030x8) || defined(STM32F098xx)
	#include "stm32f0xx_hal.h"
#else
	#include "stm32f4xx_hal.h"
#endif

#include "arm_math.h"
#include "cmsis_os.h"

// Filters
#define MAX_ORDER_NR			3

// Events:
#define MAX_EVENT_DATA_WORDS	8
#define JOB_RX_QUEUE_SIZE		8
#define MAX_EVENTS_NR			32

/* Enums and structs ---------------------------------------------------------*/

// types

typedef void (*fp_t)(void* data);

typedef union
{
	uint8_t		u8[2];
	int8_t		i8[2];

	uint16_t	u16;
	int16_t		i16;

} union16_t;

typedef union
{
	uint8_t		u8[4];
	int8_t		i8[4];

	uint16_t	u16[2];
	int16_t		i16[2];

	float		f32;			/*!< single precision variable representation */
	uint32_t	u32;			/*!< unsigned 32 bit variable representation */
	int32_t		i32;			/*!< signed 32 bit variable representation */
	fp_t		fp;				/*!< function pointer that returns void and takes
	 	 	 	 	 	 	 	 void pointer as parameter */
} union32_t;

/*
 * @brief	Union representing 64 bit object.
 */
typedef union
{
	uint64_t	u64;
	int64_t		i64;
	double		f64;

	uint32_t	u32[2];
	int32_t		i32[2];
	float		f32[2];

	uint16_t	u16[4];
	int16_t		i16[4];

	uint8_t		u8[8];
	int8_t		i8[8];
} union64_t;

/*
 * @brief	Firmware union.
 */
typedef union
{
	unsigned int firm32;
	unsigned char firm8[4];
} firmware_t;

/* Private variables ---------------------------------------------------------*/

/* Public variables ----------------------------------------------------------*/

/* Fuction prototypes --------------------------------------------------------*/

/* Function declarations -----------------------------------------------------*/



#endif /* U_TYPES_H_ */
