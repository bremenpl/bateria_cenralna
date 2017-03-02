/*
 * u_logger.h
 *
 *  Created on: 26 lis 2015
 *      Author: Lukasz
 */

#ifndef U_LOGGER_H_
#define U_LOGGER_H_

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "cmsis_os.h"
#include "u_types.h"

#if defined(STM32F103xB) || defined(STM32F105xC)
	#include "stm32f1xx_hal.h"
	#include "stm32f1xx_hal_rtc.h"
	#include "stm32f1xx_hal_uart.h"
#elif defined(STM32F098xx)
	#include "stm32f0xx_hal.h"
	#include "stm32f0xx_hal_rtc.h"
	#include "stm32f0xx_hal_uart.h"
#else
	#include "stm32f4xx_hal.h"
	#include "stm32f4xx_hal_rtc.h"
	#include "stm32f4xx_hal_uart.h"
#endif

/* Defines and macros --------------------------------------------------------*/
#define RTC_REGISTER_COOKIE		0x32F2

#if defined(STM32F103xB)
#define LOG_TEXT_SIZE			200
#define LOG_QUEUE_SIZE			16
#elif defined(STM32F098xx)
#define LOG_TEXT_SIZE			128
#define LOG_QUEUE_SIZE			8
#else
#define LOG_TEXT_SIZE			256
#define LOG_QUEUE_SIZE			16
#endif

#define LOG_NAME_LEN_MAX		100
#define LOG_NR_OF_LEVELS		7

#define CONFIG_LOG_NONE					((uint8_t)0)
#define CONFIG_LOG_UART					((uint8_t)(1 << 0))
#define CONFIG_LOG_SDCARD				((uint8_t)(1 << 1))
#define CONFIG_LOG_CDC					((uint8_t)(1 << 2))

#if defined(STM32F103xB) || defined(STM32F105xC)
	#define ID_ADDRESS						0x1FFFF7E8
#elif defined(STM32F098xx)
	#define ID_ADDRESS						0x1FFFF7AC
#else
	#define ID_ADDRESS						0x1FFF7A10
#endif

/* Enums and structs ---------------------------------------------------------*/

typedef enum
{
	e_logLevel_None 		= 0,
	e_logLevel_Csv			= 1,
	e_logLevel_Critical 	= 2,
	e_logLevel_Error 		= 3,
	e_logLevel_Warning 		= 4,
	e_logLevel_Info			= 5,
	e_logLevel_Debug		= 6,

} logLevel_t;

/*
 * @brief	reset source enumerator.
 * 			Enums are assigned bitwise in case one would like to use them as masks
 */
typedef enum
{
	e_resetSource_invalid			= 0,	// bad or unknown reset source

	e_resetSource_lowPower			= 1 << 0,	// Bit 31 LPWRRSTF: Low-power reset flag
	e_resetSource_windowWatchdog 	= 1 << 1,	// Bit 30 WWDGRSTF: Window watchdog reset flag
	e_resetSource_indWatchdog 		= 1 << 2,	// Bit 29 IWDGRSTF: Independent watchdog reset flag
	e_resetSource_softReset 		= 1 << 3,	// Bit 28 SFTRSTF: Software reset flag
	e_resetSource_porPdr 			= 1 << 4,	// Bit 27 PORRSTF: POR/PDR reset flag
	e_resetSource_pinReset 			= 1 << 5,	// Bit 26 PINRSTF: PIN reset flag,
	e_resetSource_bor 				= 1 << 6,	// Bit 25 BORRSTF: BOR reset flag

} resetSource_t;

typedef struct
{
	char text[LOG_TEXT_SIZE];
	int	size;

} logLine_t;

/* Public variables prototypes -----------------------------------------------*/
extern const char* const logLevelStrings[LOG_NR_OF_LEVELS];
extern const char* const dayOfWeekStrings[7];

/* Public function prototypes ------------------------------------------------*/
void Calendar_Init(RTC_HandleTypeDef* pRtc);
HAL_StatusTypeDef Calendar_SetDate(uint8_t date, uint8_t dayOfWeek, uint8_t month, uint8_t year);
HAL_StatusTypeDef Calendar_SetTime(uint8_t hour, uint8_t minutes, uint8_t seconds);

HAL_StatusTypeDef log_Init(UART_HandleTypeDef* uartHandle,
		uint32_t printPlace, logLevel_t logLevel, const firmware_t* firm);
HAL_StatusTypeDef log_PushLine(logLevel_t logLevel, const char* text, ...);
void log_PrintBlock(uint8_t* data, size_t blockSize);
void log_UpdateLogLevel(logLevel_t logLevel);

// functions to be overwitten
uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);

#endif /* U_LOGGER_H_ */
