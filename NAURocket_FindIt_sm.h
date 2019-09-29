#ifndef NAUROCKET_FINDIT_SM_H_INCLUDED
#define NAUROCKET_FINDIT_SM_H_INCLUDED

/***********************************/
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
	#include "NAUR_FI_f103_config.h"

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

	#define NEO6_LENGTH_MIN			160
	#define	NEO6_MAX_LENGTH			600
	#define MAX_CHAR_IN_NEO6		590

	#define FILE_NAME_SIZE 			25
	#define RX_BUFFER_SIZE 			600

	#define	TIMEZONE				3
	#define	DEBUG_STRING_SIZE		600

	//***********************************************************

	typedef struct
	{
		UART_HandleTypeDef *uart;
	}	Debug_struct;
//***********************************************************

	typedef struct
	{
		char 		string[NEO6_MAX_LENGTH];
		int 		length_int;
	}	NEO6_struct;
//***********************************************************

	typedef struct
	{
		TCHAR 		file_name_char[FILE_NAME_SIZE];
		int			file_name_int;
		FRESULT 	write_status;
		DWORD		file_size;
	}	SD_Card_struct;
//***********************************************************

	typedef struct
	{
		uint8_t 	end_of_UART_packet		;
		uint8_t 	shudown_button_pressed	;
		uint8_t		no_signal				;
	}	Flags_struct;
//***********************************************************
//***********************************************************

	FRESULT fres;
	GPS_state_machine sm_stage;
	RESULT_ENUM result;
	NEO6_struct NEO6;
	SD_Card_struct SD;
	RingBuffer_DMA rx_buffer;
	Flags_struct FLAG;
	Debug_struct DebugH;

//***********************************************************

	void NAUR_Init (void);
	void NAUR_Main (void);

	void Update_flag_Shudown_button_pressed(void)	;
	void Update_flag_End_of_UART_packet(void)	;
	void Update_No_signal(void);

	void TIM3_end_of_packet_Reset(void);

//***********************************************************
#endif 	//	NAUROCKET_FINDIT_SM_H_INCLUDED
