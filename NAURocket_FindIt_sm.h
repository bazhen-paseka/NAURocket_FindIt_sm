#ifndef NAUROCKET_FINDIT_SM_H_INCLUDED
#define NAUROCKET_FINDIT_SM_H_INCLUDED

/***********************************/
#include <NAURocket_FindIt_local_config.h>
#include "main.h"
#include "dma.h"
#include "fatfs.h"
#include "iwdg.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

	#include <stdio.h>
	#include <string.h>
	#include "lcd.h"
	#include "ringbuffer_dma.h"
	
//***********************************************************

	typedef enum
	{
		R_FAILED,
		R_OK
	} RESULT_ENUM;
//***********************************************************

	typedef enum
	{
		SM_START					,
		SM_READ_FROM_RINGBUFFER		,
		SM_CHECK_FLAGS				,
		SM_PREPARE_FILENAME			,
		SM_WRITE_SDCARD				,
		SM_PRINT_ALL_INFO			,
		SM_FINISH
	} GPS_state_machine;
//***********************************************************

	typedef enum
	{
		GPS_CH_0					,
		GPS_CH_1					,
		GPS_CH_2					,
		GPS_CH_3					,
		GPS_CH_QNT
	} GPS_channel;
//***********************************************************

	#define	GPS_MAX_LENGTH			850
	#define MAX_CHAR_IN_GPS			(GPS_MAX_LENGTH - 1)

	#define FILE_NAME_SIZE 			 25
	#define RX_BUFFER_SIZE 			0xFF

	#define	DEBUG_STRING_SIZE		850

	//***********************************************************

	typedef struct {
		UART_HandleTypeDef *uart	;
	} Debug_struct;
//***********************************************************

	typedef struct {
		GPS_channel		channel					;
		char 			string[GPS_MAX_LENGTH]	;
		int 			length_int				;
		uint8_t 		end_of_UART_packet		;
		uint8_t 		packet_overflow			;
		uint8_t			no_signal				;
		uint8_t 		time_overflow_u8		;
		uint8_t			UART_packet_ready_u8	;
	} GPS_struct;
//***********************************************************

	typedef struct {
		TCHAR 		file_name_char[FILE_NAME_SIZE]	;
		int			file_name_int					;
		FRESULT 	write_status					;
		DWORD		file_size						;
		uint8_t 	shudown_button_pressed			;
	} SD_Card_struct;
//***********************************************************
//***********************************************************

	FRESULT 				fres					;
	GPS_state_machine 		sm_stage				;
	RESULT_ENUM 			result					;
	GPS_struct 				GPS[GPS_CH_QNT]			;
	SD_Card_struct 			SD						;
	RingBuffer_DMA 			rx_buffer[GPS_CH_QNT]	;
	Debug_struct 			DebugH					;

//***********************************************************

	void NAUR_Init (void)						;
	void NAUR_Main (void)						;

	void Set_flag_End_of_UART_2_packet(void)	;
	void Set_flag_End_of_UART_3_packet(void)	;
	void RTU_2_reset(void)						;
	void RTU_3_reset(void)						;

	void Set_flag_No_signal(void)				;
	void Set_flag_time_overflow_package(void)	;
	void Set_flag_Shudown_button_pressed(void)	;

//***********************************************************
#endif 	//	NAUROCKET_FINDIT_SM_H_INCLUDED
