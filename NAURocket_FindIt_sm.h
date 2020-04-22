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
	  FLAG_RESET = 0,
	  FLAG_SET
	} Flag_state;
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
		Flag_state 		end_of_UART_packet_flag	;
		Flag_state 		packet_overflow_flag	;
		Flag_state		no_signal_flag			;
		Flag_state 		time_overflow_flag		;
		Flag_state		UART_packet_ready_flag	;
	} GPS_struct;
//***********************************************************

	typedef struct {
		TCHAR 		file_name_char[FILE_NAME_SIZE]	;
		int			file_name_int					;
		FRESULT 	write_status_fr					;
		DWORD		file_size						;
		Flag_state 	shudown_button_pressed_flag		;
	} SD_Card_struct;
//***********************************************************
//***********************************************************

	FRESULT 				fres					;
	GPS_state_machine 		sm_stage				;
	RESULT_ENUM 			result					;
	GPS_struct 				GPS[GPS_CH_QNT]			;
	SD_Card_struct 			SD						;
	RingBuffer_DMA 			rx_buffer[GPS_CH_QNT]	;
	Debug_struct 			Debug_ch				;

//***********************************************************

	void NAUR_Init (void)						;
	void NAUR_Main (void)						;

	void Set_flag_End_of_UART_packet(GPS_channel _channel)	;
	void RTU_2_reset(void)						;
	void RTU_3_reset(void)						;

	void Set_flag_No_signal(void)				;
	void Set_flag_time_overflow_package(void)	;
	void Set_flag_Shudown_button_pressed(void)	;

//***********************************************************
#endif 	//	NAUROCKET_FINDIT_SM_H_INCLUDED
