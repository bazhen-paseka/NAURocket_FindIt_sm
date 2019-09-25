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
		SM_FIND_GGA					,
		SM_FIND_ASTERISK			,
		SM_CALC_SHECKSUM			,
		SM_PREPARE_FILENAME			,
		SM_WRITE_SDCARD				,
		SM_PRINT_ALL_INFO			,
		SM_FINISH					,
		SM_ERROR_HANDLER			,
		SM_SHUTDOWN					,
		SM_READ_FROM_SDCARD
	} GPS_state_machine;

	//***********************************************************

	#define NEO6_LENGTH_MIN			160
	#define	NEO6_MAX_LENGTH			650
	#define MAX_CHAR_IN_NEO6		630

	#define GGA_STRING_MAX_SIZE 	99
	#define GGA_FORCE_START			103
	#define GGA_FORCE_LENGTH		75
	#define GGA_LENGTH_MIN			33

	#define FILE_NAME_SIZE 			25
	#define RX_BUFFER_SIZE 			550

	#define	TIMEZONE				3
	#define	DEBUG_STRING_SIZE		650


	//***********************************************************

	typedef struct
	{
		UART_HandleTypeDef *uart;
	}	Debug_struct;

	//***********************************************************

	typedef struct
	{
		int 		hour_int	;
		int 		minutes_int	;
		int 		seconds_int	;
		uint8_t		updated_flag;
	} 	Time_struct;

	//***********************************************************

	typedef struct
	{
		char		string[GGA_STRING_MAX_SIZE];
		int			Neo6_start;
		int			Neo6_end;
		int			length;
		uint8_t 	beginning_chars;
		uint8_t 	ending_char;
	}	GGA_struct;

	//***********************************************************

	typedef struct
	{
		char 		string[NEO6_MAX_LENGTH];
		int 		length_int;
	}	NEO6_struct;

	//***********************************************************

	typedef struct
	{
		uint8_t 	calc_u8				;
		uint8_t 	glue_u8				;
		uint8_t 	status_flag			;
	}	CheckSum_struct;

	//***********************************************************

	typedef struct
	{
		TCHAR 		filename[FILE_NAME_SIZE];
		FRESULT 	write_status;
	}	SD_Card_struct;

	//***********************************************************

	typedef struct
	{
		uint8_t 	end_of_UART_packet		;
		uint8_t 	shudown_button_pressed	;
		uint8_t		no_signal				;
		uint8_t		no_signal_cnt			;
		uint32_t	received_packet_cnt_u32 ;
		uint32_t	correct_packet_cnt_u32	;
	}	Flags_struct;

	//***********************************************************

//	char DebugString[DEBUG_STRING_SIZE];

//***********************************************************

	FRESULT fres;
	GPS_state_machine sm_stage;
	RESULT_ENUM result;
	Time_struct Time;
	GGA_struct GGA;
	NEO6_struct NEO6;
	CheckSum_struct CS;
	SD_Card_struct SD;
	RingBuffer_DMA rx_buffer;
	Flags_struct FLAG;
	Debug_struct DebugH;


//***********************************************************

	void NAURocket_FindIt_Init (void);
	void NAURocket_FindIt_Main (void);

	void Clear_variables(NEO6_struct * _neo6, GGA_struct * _gga, CheckSum_struct * _cs, Flags_struct * _flag);
	void Read_from_RingBuffer(NEO6_struct * _neo6, RingBuffer_DMA * buffer, Flags_struct * _flag);

	uint8_t Find_Begin_of_GGA_string(NEO6_struct*, GGA_struct* );
	uint8_t Find_End_of_GGA_string(NEO6_struct*, GGA_struct*);
	uint8_t Calc_SheckSum_GGA(GGA_struct * _gga, CheckSum_struct * _cs, Flags_struct * _flag);

	void Copy_GGA_Force(NEO6_struct * _neo6, GGA_struct * _gga);
	void Copy_GGA_Correct(NEO6_struct * _neo6, GGA_struct * _gga);

	void Get_time_from_GGA_string(GGA_struct* , Time_struct * _time);
	void Increment_time(Time_struct *);

	void Prepare_filename(CheckSum_struct * _cs, Time_struct * _time, SD_Card_struct * _sd);
	void Write_SD_card(GGA_struct * _gga, SD_Card_struct * _sd);

	void Print_all_info(NEO6_struct * _neo6, GGA_struct * _gga, CheckSum_struct * _cs, Time_struct * _time, SD_Card_struct * _sd, Flags_struct * _flag);
	void Print_No_signal(Flags_struct * _flag);

	void Update_flag_shudown_button_pressed(void)	;
	void Update_flag_end_of_UART_packet(void)	;
	void Update_No_Signal(void);

	void TIM3_end_of_packet_Start(void);
	void TIM3_end_of_packet_Stop(void);
	void TIM3_end_of_packet_Reset(void);

	void TIM4_no_signal_Start(void);
	void TIM4_no_signal_Stop(void);
	void TIM4_no_signal_Reset(void);

	void Beep(void);
	void ShutDown(void);

	void Read_SD_card(void);

//***********************************************************
#endif 	//	NAUROCKET_FINDIT_SM_H_INCLUDED
