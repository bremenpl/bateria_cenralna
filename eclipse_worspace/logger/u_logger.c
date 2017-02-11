/*
 * u_logger.c
 *
 *  Created on: 26 lis 2015
 *      Author: Lukasz
 */


/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>

#include "u_logger.h"

/* Defines and macros --------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
RTC_HandleTypeDef* p_rtcHandle;

osMessageQId msgQId_LogLines;
osThreadId logQueueTaskHandle;

static UART_HandleTypeDef* p_logUartHandle;
static bool logQueueInitialized = false;
static osMutexId log_CreateStringMutexId;
static osMutexId log_WriteSdMutexId;

uint32_t log_where2Print;
logLevel_t log_globalLevel;


/* Public variables ----------------------------------------------------------*/

const char* const logLevelStrings[LOG_NR_OF_LEVELS] =
{
	"None",
	"Csv",
	"Critical",
	"Error",
	"Warning",
	"Info",
	"Debug",
};

const char* const dayOfWeekStrings[7] =
{
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday",
	"Saturday",
	"Sunday"
};

/* Private function prototypes -----------------------------------------------*/
void task_LogDequeue(void const* argument);
static HAL_StatusTypeDef Calendar_SetDefaultDateAndTime();
void System_ReadUniqueId(uint32_t* buffer);

/* Public function definitions -----------------------------------------------*/
void Calendar_Init(RTC_HandleTypeDef* pRtc)
{
	assert_param(pRtc);
	p_rtcHandle = pRtc;

	// Check if Data stored in BackUp register1: No Need to reconfigure RTC
	if (HAL_RTCEx_BKUPRead(p_rtcHandle, RTC_BKP_DR1) != RTC_REGISTER_COOKIE)
		Calendar_SetDefaultDateAndTime();
	else
	{
		/* Clear source Reset Flag */
		__HAL_RCC_CLEAR_RESET_FLAGS();
	}
}

HAL_StatusTypeDef Calendar_SetDate(uint8_t dayOfWeek, uint8_t date, uint8_t month, uint8_t year)
{
	RTC_DateTypeDef sdatestructure;

	/*##-1- Configure the Date #################################################*/
	if (date >= 1 && date <= 31)
		sdatestructure.Date = date;
	else return HAL_ERROR;

	if (dayOfWeek >= RTC_WEEKDAY_MONDAY && dayOfWeek <= RTC_WEEKDAY_SUNDAY)
		sdatestructure.WeekDay = dayOfWeek;
	else return HAL_ERROR;

	if (month >= RTC_MONTH_JANUARY && month <= RTC_MONTH_DECEMBER)
		sdatestructure.Month = month;
	else return HAL_ERROR;

	if (year >= 0 && year <= 99)
		sdatestructure.Year = year;
	else return HAL_ERROR;


	if(HAL_RTC_SetDate(p_rtcHandle, &sdatestructure, RTC_FORMAT_BIN) != HAL_OK)
		return HAL_ERROR;

	/*##-3- Writes a data in a RTC Backup data Register1 #######################*/
	HAL_RTCEx_BKUPWrite(p_rtcHandle, RTC_BKP_DR1, RTC_REGISTER_COOKIE);

	return HAL_OK;
}

HAL_StatusTypeDef Calendar_SetTime(uint8_t hour, uint8_t minutes, uint8_t seconds)
{
	RTC_TimeTypeDef stimestructure;

	/*##-2- Configure the Time #################################################*/
#ifdef STM32F407xx
	stimestructure.DayLightSaving = RTC_DAYLIGHTSAVING_NONE ;
	stimestructure.StoreOperation = RTC_STOREOPERATION_RESET;
#endif

	if (hour >= 0 && hour <= 23)
		stimestructure.Hours = hour;
	else return HAL_ERROR;

	if (minutes >= 0 && minutes <= 59)
		stimestructure.Minutes = minutes;
	else return HAL_ERROR;

	if (seconds >= 0 && seconds <= 59)
		stimestructure.Seconds = seconds;
	else return HAL_ERROR;

	if (HAL_RTC_SetTime(p_rtcHandle, &stimestructure, RTC_FORMAT_BIN) != HAL_OK)
		return HAL_ERROR;

	/*##-3- Writes a data in a RTC Backup data Register1 #######################*/
	HAL_RTCEx_BKUPWrite(p_rtcHandle, RTC_BKP_DR1, RTC_REGISTER_COOKIE);

	return HAL_OK;
}

static HAL_StatusTypeDef Calendar_SetDefaultDateAndTime()
{
	if (Calendar_SetDate(RTC_WEEKDAY_MONDAY, 1, 1, 15) != HAL_OK)
		return HAL_ERROR;

	if (Calendar_SetTime(0, 0, 0) != HAL_OK)
		return HAL_ERROR;

	return HAL_OK;
}

/**
 * @brief	Configures the logger rtos queue.
 * @param	uartHandle: Handler to an uart structure.
 * @param	printPlace: None, sd card or uart port.
 * @param	logLevel: Loging level
 */
void log_Init(UART_HandleTypeDef* uartHandle,
		uint32_t printPlace, logLevel_t logLevel, const firmware_t* firm)
{
	assert_param(uartHandle);
	assert_param(firm);

	p_logUartHandle = uartHandle;

	osMessageQDef_t msgDef_LogLines;
	msgDef_LogLines.item_sz = sizeof(logLine_t*); // zmiana na rozmiar wskaznika
	msgDef_LogLines.queue_sz = LOG_QUEUE_SIZE;
	msgQId_LogLines = osMessageCreate(&msgDef_LogLines, NULL);

	osMutexDef_t logMutexDef1;
	log_CreateStringMutexId = osMutexCreate(&logMutexDef1);

	osMutexDef_t logMutexDef2;
	log_WriteSdMutexId = osMutexCreate(&logMutexDef2);

	osThreadDef(logQueueTask, task_LogDequeue, osPriorityLow, 0, 512);
	logQueueTaskHandle = osThreadCreate(osThread(logQueueTask), NULL);
	assert_param(logQueueTaskHandle);

	logQueueInitialized = true;

	// ---------------------------------------------------------------

	RTC_TimeTypeDef time;
	RTC_DateTypeDef date;

	// Get current time:
	HAL_RTC_GetTime(p_rtcHandle, &time, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(p_rtcHandle, &date, RTC_FORMAT_BIN);

	// Get device ID:
	uint32_t uDevId[3];
	System_ReadUniqueId(uDevId);

	// lock logging flags. If user changes flash config it will affect this only after restart.
	log_where2Print = printPlace;

	// set log level
	log_UpdateLogLevel(logLevel);

#ifdef DEBUG
	char* build = "Debug";
#else
	char* build = "Release";
#endif

	// Print information
	log_PushLine(e_logLevel_Info, "Build: (%s build) "
			"log level %s(%u).",
			build, logLevelStrings[log_globalLevel], log_globalLevel);
	log_PushLine(e_logLevel_Info, "Firmware version: %u.%u.%u.%u",
			firm->firm8[3], firm->firm8[2], firm->firm8[1], firm->firm8[0]);
	log_PushLine(e_logLevel_Info, "Board unique device ID: 0x%X 0x%X 0x%X",
			uDevId[2], uDevId[1], uDevId[0]);
	log_PushLine(e_logLevel_Info, "Date: %s %02u.%02u.%02u. Time: %02u:%02u:%02u",
			dayOfWeekStrings[date.WeekDay - 1], date.Date, date.Month, date.Year + 2000,
			time.Hours, time.Minutes, time.Seconds);
}

/*
 * @brief	Sets the logLevel for the logger
 * @param	logLevel: Log level to be set
 */
void log_UpdateLogLevel(logLevel_t logLevel)
{
	// trim the log level if the number is to high
	if (logLevel > LOG_NR_OF_LEVELS)
		logLevel = LOG_NR_OF_LEVELS - 1;

	log_globalLevel = logLevel;
}

/*
 * @brief	Puts characters line into the log queue.
 * @param	text: pointer to a character string.
 * @param	... argptr parameters
 *
 * @return	HAL_OK if queue initialised.
 */
HAL_StatusTypeDef log_PushLine(logLevel_t logLevel, const char* text, ...)
{
	assert_param(text);

	if (!logQueueInitialized)
		return HAL_ERROR;

	if ((CONFIG_LOG_NONE == log_where2Print) || (logLevel > log_globalLevel))
		return HAL_OK;

	static logLine_t logBuffer[LOG_QUEUE_SIZE];
	static uint32_t bufferIndex = 0;

	osMutexWait(log_CreateStringMutexId, osWaitForever);
	if (++bufferIndex >= LOG_QUEUE_SIZE)
		bufferIndex = 0;
	osMutexRelease(log_CreateStringMutexId);

	uint32_t bufferIndexLocked = bufferIndex;
	int len = 0;

	va_list argptr;
	va_start(argptr, text);

	if (e_logLevel_Csv != logLevel)
	{
		RTC_TimeTypeDef timeStruct;
		RTC_DateTypeDef dateStruct;
		HAL_RTC_GetTime(p_rtcHandle, &timeStruct, RTC_FORMAT_BIN);
		HAL_RTC_GetDate(p_rtcHandle, &dateStruct, RTC_FORMAT_BIN); // only to unlock registers
		unsigned int msec = 0;

#ifdef STM32F407xx
		msec = 999 - (timeStruct.SubSeconds * 999 / timeStruct.SecondFraction);
#endif

		len = sprintf(logBuffer[bufferIndexLocked].text, "[%02u:%02u:%02u.%03u][%s] ",
				timeStruct.Hours, timeStruct.Minutes, timeStruct.Seconds, msec,
				logLevelStrings[logLevel]);
	}

	/**
	 * 	Fill the buffer up to LOG_TEXT_SIZE - len - 2 size because the 2
	 * 	bytes needed are needed for termination characters later
	 */
	len += vsnprintf(logBuffer[bufferIndexLocked].text + len, LOG_TEXT_SIZE - len - 2, text, argptr);
	va_end(argptr);

	logBuffer[bufferIndexLocked].size = len;
	if (osOK != osMessagePut(msgQId_LogLines, (uint32_t)&logBuffer[bufferIndexLocked], 0))
		assert_param(0); // cant log...

	return HAL_OK;
}

void task_LogDequeue(void const* argument)
{
	// declarations:
	//portTickType xLastWakeTime;
	//xLastWakeTime = xTaskGetTickCount();

	osEvent retEvent;

	while(1)
	{
		retEvent = osMessageGet(msgQId_LogLines, osWaitForever);

		if (retEvent.status == osEventMessage)
		{
			logLine_t* log = (logLine_t*)retEvent.value.p;
			assert_param(log);

			log->text[log->size++] = '\r';
			log->text[log->size++] = '\n';

			if (log_where2Print & CONFIG_LOG_UART)
			{
				while (HAL_UART_GetState(p_logUartHandle) != HAL_UART_STATE_READY);
				HAL_UART_Transmit_DMA(p_logUartHandle, (uint8_t*)log->text, log->size);
			}

			if (log_where2Print & CONFIG_LOG_CDC)
				CDC_Transmit_FS((uint8_t*)log->text, log->size);

			// minimum save to save time
			//vTaskDelayUntil( &xLastWakeTime, ( 20 / portTICK_RATE_MS ) );
		}
		else
		{
			// error msg
		}
 	}
}

void log_PrintBlock(uint8_t* data, size_t blockSize)
{
	assert_param(data);
	size_t nrOfLines = blockSize / sizeof(uint64_t);
	size_t lastLineSize = 0;

	if (!nrOfLines)
		nrOfLines = 1;
	else
	{
		lastLineSize = blockSize % sizeof(uint64_t);
		if (lastLineSize)
			nrOfLines++;
	}

	uint64_t dataLine;
	size_t i;
	for (i = 0; i < nrOfLines; i++)
	{
		dataLine = 0;

		if (i < (nrOfLines - 1))
			memcpy(&dataLine, data + (sizeof(uint64_t) * i), sizeof(uint64_t));
		else
			memcpy(&dataLine, data + (sizeof(uint64_t) * i), lastLineSize);

		log_PushLine(e_logLevel_Debug, "Data block %u = 0x%lX%lX",
				i, (uint32_t)(dataLine >> 32),(uint32_t)dataLine);

	}
}

void System_ReadUniqueId(uint32_t* buffer)
{
	assert_param(buffer);
	assert_param(buffer + 1);
	assert_param(buffer + 2);

	HAL_FLASH_Unlock();

	buffer[0] = (*(__IO uint32_t*) ID_ADDRESS);
	buffer[1] = (*(__IO uint32_t*) (ID_ADDRESS + 4));
	buffer[2] = (*(__IO uint32_t*) (ID_ADDRESS + 8));

	HAL_FLASH_Lock();
}

// functions to be overwitten

/*
 * @brief	USB CDC logging writer
 * @param	buf: pointer to data buffer.
 * @param	Len: buffer length
 */
__attribute__((weak)) uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len)
{
	// always return 1 indicating error if not ovewritten
	return 1;
}

















