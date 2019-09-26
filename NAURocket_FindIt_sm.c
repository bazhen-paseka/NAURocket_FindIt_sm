
#include "main.h"
#include "NAURocket_FindIt_sm.h"

//***********************************************************

	uint8_t rx_circular_buffer[RX_BUFFER_SIZE];
	extern DMA_HandleTypeDef hdma_usart3_rx;
	char DebugString[DEBUG_STRING_SIZE];

//***********************************************************
//***********************************************************

	void Clear_variables(NEO6_struct * _neo6, GGA_struct * _gga, CheckSum_struct * _cs, Flags_struct * _flag, SD_Card_struct * _sd);
	void Read_from_RingBuffer(NEO6_struct * _neo6, RingBuffer_DMA * buffer, Flags_struct * _flag);

	uint8_t Find_Begin_of_GGA_string(NEO6_struct*, GGA_struct* );
	uint8_t Find_End_of_GGA_string(NEO6_struct*, GGA_struct*);
	uint8_t Calc_SheckSum_GGA(GGA_struct * _gga, CheckSum_struct * _cs, Flags_struct * _flag);

	void Copy_GGA_Force(NEO6_struct * _neo6, GGA_struct * _gga);
	void Copy_GGA_Correct(NEO6_struct * _neo6, GGA_struct * _gga);

	void Get_time_from_GGA_string(GGA_struct* , Time_struct * _time, SD_Card_struct * _sd);
	void Increment_time(Time_struct * _time);

	void Prepare_filename(CheckSum_struct * _cs, Time_struct * _time, SD_Card_struct * _sd);
	void Write_SD_card(GGA_struct * _gga, SD_Card_struct * _sd);

	void Print_all_info(NEO6_struct * _neo6, GGA_struct * _gga, CheckSum_struct * _cs, Time_struct * _time, SD_Card_struct * _sd, Flags_struct * _flag);
	void Print_No_signal(Flags_struct * _flag);

	void TIM3_end_of_packet_Start(void);
	void TIM3_end_of_packet_Stop(void);

	void TIM4_no_signal_Start(void);
	void TIM4_no_signal_Stop(void);
	void TIM4_no_signal_Reset(void);

	void Beep(void);
	void ShutDown(void);

	void Read_SD_card(void);

//***********************************************************
//***********************************************************

void NAUR_Init (void)
{
	LCD_Init();
	LCD_SetRotation(1);

	LCD_SetCursor(0, 0);

#if (NAUR_FI_F446 == 1)
	LCD_FillScreen(ILI92_BLACK);
	LCD_SetTextColor(ILI92_GREEN, ILI92_BLACK);
#elif (NAUR_FI_F103 == 1)
	LCD_FillScreen(ILI92_WHITE);
	LCD_SetTextColor(ILI92_BLACK, ILI92_WHITE);
#endif

#if (NAUR_FI_F446 == 1)
	DebugH.uart = &huart5;
#elif (NAUR_FI_F103 == 1)
	DebugH.uart = &huart2;
#endif

	sprintf(DebugString,"\r\n\r\n");
	HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);

#if (NAUR_FI_F446 == 1)
	sprintf(DebugString,"NAUR Find It F446\r\n2019 v2.1.0\r\nfor_debug UART5 115200/8-N-1\r\n");
#elif (NAUR_FI_F103 == 1)
	sprintf(DebugString,"NAUR Find It F103\r\n2019 v2.1.0\r\nfor_debug UART5 115200/8-N-1\r\n");
#endif
	HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
	LCD_Printf("%s",DebugString);

	RingBuffer_DMA_Init(&rx_buffer, &hdma_usart3_rx, rx_circular_buffer, RX_BUFFER_SIZE);  	// Start UART receive
	HAL_UART_Receive_DMA(&huart3, rx_circular_buffer, RX_BUFFER_SIZE);  	// how many bytes in buffer

#if (NAUR_FI_F446 == 1)
	FATFS_SPI_Init(&hspi1);	/* Initialize SD Card low level SPI driver */
#elif (NAUR_FI_F103 == 1)
	FATFS_SPI_Init(&hspi2);	/* Initialize SD Card low level SPI driver */
#endif

	do
	{
		fres = f_mount(&USERFatFS, "", 1);	/* try to mount SDCARD */
		if (fres == FR_OK)
		{
			sprintf(DebugString,"\r\nSDcard mount - Ok \r\n");
			HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
			LCD_Printf("%s",DebugString);
		}
		else
		{
			f_mount(NULL, "", 0);			/* Unmount SDCARD */
			Error_Handler();
			sprintf(DebugString,"\r\nSDcard mount: Failed. Error: %d            \r\n", fres);
			HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
			LCD_Printf("%s",DebugString);
			HAL_Delay(1000);
		}
	}
	#if (DEBUG_MODE == 1)
		while (0);
	#else
		while (fres !=0);
	#endif

	////// не знайщов HAL_DMA_STATE_ERROR
//		if (hdma_usart3_rx.State == HAL_DMA_STATE_ERROR)
//		{
//			HAL_UART_DMAStop(&huart3);
//			HAL_UART_Receive_DMA(&huart3, rx_circular_buffer, RX_BUFFER_SIZE);
//		}
  	//////

#if (NAUR_FI_F446 == 1)
	LCD_FillScreen(ILI92_BLACK);
#elif (NAUR_FI_F103 == 1)
	LCD_FillScreen(ILI92_WHITE);
#endif
	TIM4_no_signal_Start();
	HAL_IWDG_Refresh(&hiwdg);
}

//***********************************************************
//***********************************************************

void NAUR_Main (void)
{
	switch (sm_stage)
	{
		case SM_START:
		{
			Clear_variables(&NEO6, &GGA, &CS, &FLAG, &SD);
			TIM3_end_of_packet_Reset();
			TIM3_end_of_packet_Start();
			sm_stage =SM_READ_FROM_RINGBUFFER;
		} break;

		//***********************************************************

		case SM_READ_FROM_RINGBUFFER:
		{
			Read_from_RingBuffer(&NEO6, &rx_buffer, &FLAG);
			sm_stage = SM_CHECK_FLAGS;
		} break;

		//***********************************************************

		case SM_CHECK_FLAGS:
		{
			if (FLAG.end_of_UART_packet == 1)
			{
				TIM3_end_of_packet_Stop();
				if (NEO6.length_int > NEO6_LENGTH_MIN)
				{
					HAL_GPIO_WritePin(TEST_PA12_GPIO_Port, TEST_PA12_Pin, GPIO_PIN_SET);
					FLAG.received_packet_cnt_u32++;
					TIM4_no_signal_Reset();
					sm_stage = SM_FIND_GGA;
					break;
				}
				else
				{
					FLAG.end_of_UART_packet = 0;
					sm_stage = SM_START;
					break;
				}
			}

			if (FLAG.shudown_button_pressed == 1)
			{
				sm_stage = SM_SHUTDOWN;
				break;
			}

			if (FLAG.no_signal == 1)
			{
				Print_No_signal(&FLAG);
				sm_stage = SM_FINISH;
				break;
			}

				//	if (time_read_from_SD_u8 == 1)
				//	{
				//		sm_stage =SM_READ_FROM_SDCARD;
				//		break;
				//	}
			sm_stage = SM_READ_FROM_RINGBUFFER;
		} break;
	//***********************************************************

		case SM_FIND_GGA:
		{
			result = Find_Begin_of_GGA_string(&NEO6, &GGA);
			if ( result == R_OK)
			{
				sm_stage = SM_FIND_ASTERISK;
			}
			else
			{
				Copy_GGA_Force(&NEO6, &GGA);
				sm_stage = SM_PREPARE_FILENAME;
			}
		} break;
	//***********************************************************

		case SM_FIND_ASTERISK:
		{
			result = Find_End_of_GGA_string(&NEO6, &GGA);
			if ( result == R_OK )
			{
				Copy_GGA_Correct(&NEO6, &GGA);
				sm_stage = SM_CALC_SHECKSUM;
			}
			else
			{
				Copy_GGA_Force(&NEO6, &GGA);
				sm_stage = SM_PREPARE_FILENAME;
			}
		} break;
	//***********************************************************

		case SM_CALC_SHECKSUM:
		{
			result = Calc_SheckSum_GGA(&GGA, &CS, &FLAG);
			if ( result == R_OK )
			{
				Get_time_from_GGA_string(&GGA, &Time, &SD);
			}
			sm_stage = SM_PREPARE_FILENAME;
		} break;
	//***********************************************************

		case SM_PREPARE_FILENAME:
		{
			Increment_time(&Time);
			Prepare_filename(&CS, &Time, &SD);
			sm_stage = SM_WRITE_SDCARD;
		} break;
	//***********************************************************

		case SM_WRITE_SDCARD:
		{
			Write_SD_card(&GGA, &SD);
			sm_stage = SM_PRINT_ALL_INFO;
		} break;
		//***********************************************************

		case SM_PRINT_ALL_INFO:
		{
			Print_all_info(&NEO6, &GGA, &CS, &Time, &SD, &FLAG);
			sm_stage = SM_FINISH;
		} break;
		//***********************************************************

		case SM_FINISH:
		{
			FLAG.end_of_UART_packet  = 0 ;
			HAL_GPIO_WritePin(TEST_PA12_GPIO_Port, TEST_PA12_Pin, GPIO_PIN_RESET);
			sm_stage = SM_START;
		} break;
		//***********************************************************

		case SM_ERROR_HANDLER:
		{
			LCD_SetCursor(0, 0);
			sprintf(DebugString,"Buf empty. L= %d\r\n", NEO6.length_int);
			HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
			LCD_Printf("%s", DebugString);
			NEO6.length_int = 0;

			sm_stage = SM_FINISH;
		} break;
		//***********************************************************

		case SM_SHUTDOWN:
		{
			TIM3_end_of_packet_Stop();
			ShutDown();
			FLAG.shudown_button_pressed = 0;
			sm_stage = SM_START;
		} break;
		//***********************************************************

		case SM_READ_FROM_SDCARD:
		{
//			Read_SD_card();
//			time_read_from_SD_u8 = 0;
//			sm_stage = SM_START;
		} break;
		//***********************************************************

		default:
		{
			sm_stage = SM_START;
		} break;
	}	//			switch (sm_stage)
}
//***********************************************************

void Print_all_info(NEO6_struct * _neo6, GGA_struct * _gga, CheckSum_struct * _cs, Time_struct * _time, SD_Card_struct * _sd, Flags_struct * _flag)
{
	sprintf(DebugString,"%s", _gga->string);
	HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);

	LCD_SetCursor(0, 95*(_time->seconds_int%2));
	LCD_Printf("%s", DebugString);

	sprintf(DebugString,"%d/%d/%d/cs%d; pct: %d/%d/%d; %s; SD_write: %d; size: %d\r\n",
								_gga->Neo6_start,
								_gga->Neo6_end,
								_gga->length,
								_cs->status_flag,
								(int)_flag->received_packet_cnt_u32,
								(int)_flag->correct_packet_cnt_u32,
								(int)(_flag->received_packet_cnt_u32 - _flag->correct_packet_cnt_u32),
								_sd->filename,
								_sd->write_status,
								(int)_sd->file_size);
	HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);

	sprintf(DebugString,"%s SD_wr %d\r\n",
								_sd->filename,
								_sd->write_status);


	LCD_SetCursor(0, 200);
	LCD_Printf("%s", DebugString);

}
//***********************************************************

void TIM3_end_of_packet_Start(void)
{
	HAL_TIM_Base_Start_IT(&htim3);
	HAL_TIM_Base_Start(&htim3);
}
//***********************************************************

void TIM3_end_of_packet_Stop(void)
{
	HAL_TIM_Base_Stop_IT(&htim3);
	HAL_TIM_Base_Stop(&htim3);
}
//***********************************************************

void TIM3_end_of_packet_Reset(void)
{
	TIM3->CNT = 0;
}

//***********************************************************
void TIM4_no_signal_Start(void)
{
	HAL_TIM_Base_Start_IT(&htim4);
	HAL_TIM_Base_Start(&htim4);
}
//***********************************************************

void TIM4_no_signal_Stop(void)
{
	HAL_TIM_Base_Stop_IT(&htim4);
	HAL_TIM_Base_Stop(&htim4);
}
//***********************************************************

void TIM4_no_signal_Reset(void)
{
	TIM4->CNT = 0;
}
//***********************************************************


void Increment_time(Time_struct * _time)
{
	if (_time->seconds_int < 59)
	{
		_time->seconds_int++;
		return;
	}

	_time->seconds_int = 0;
	if (_time->minutes_int < 59)
	{
		_time->minutes_int++;
		return;
	}

	_time->minutes_int = 0;
	_time->hour_int++;
}
//***********************************************************

uint8_t Find_Begin_of_GGA_string(NEO6_struct* _neo6, GGA_struct* _gga )
{
	for (int i=6; i<= _neo6->length_int; i++)
	{
		if (memcmp(&_neo6->string[i-6], "$GPGGA," ,7) == 0)
		{
			_gga->Neo6_start = i - 6;
			_gga->beginning_chars = 1;
			return 1;
		}
	}
	return 0;
}
//***********************************************************

uint8_t Find_End_of_GGA_string(NEO6_struct* _neo6, GGA_struct* _gga)
{
	if (_gga->beginning_chars == 0)
	{
		return 0;
	}

	for (int i=_gga->Neo6_start; i<=_neo6->length_int; i++)
	{
		if (_neo6->string[i] == '*')
		{
			_gga->Neo6_end = i + 5;
			_gga->length = _gga->Neo6_end - _gga->Neo6_start;
			if ((_gga->length < GGA_LENGTH_MIN) || (_gga->length > GGA_STRING_MAX_SIZE))
			{
				return 0;
			}
			_gga->ending_char = 1;
			return 1;
		}
	}
	return 0;
}
//***********************************************************

void Get_time_from_GGA_string(GGA_struct * _gga, Time_struct * _time, SD_Card_struct * _sd)
{
	if (_time->updated_flag == 1)
	{
		return;
	}

	_time->updated_flag = 1;

	#define TIME_SIZE	5
	char time_string[TIME_SIZE];

	memset(time_string,0,TIME_SIZE);

	memcpy(time_string, &_gga->string[7], 2);
	_time->hour_int = atoi(time_string) + TIMEZONE;

	memcpy(time_string, &_gga->string[9], 2);
	_time->minutes_int = atoi(time_string);

	memcpy(time_string, &_gga->string[11], 2);
	_time->seconds_int = atoi(time_string);

	_sd->file_name_int = _time->hour_int*10000 + _time->minutes_int*100 + _time->seconds_int ;
}
//***********************************************************

uint8_t Calc_SheckSum_GGA(GGA_struct * _gga, CheckSum_struct * _cs, Flags_struct * _flag)
{
	#define CS_SIZE	4
	char cs_glue_string[CS_SIZE];

	//	glue:

	memset(cs_glue_string, 0, CS_SIZE);
	memcpy(cs_glue_string, &_gga->string[_gga->length - 4], 2);
	_cs->glue_u8 = strtol(cs_glue_string, NULL, 16);

	//	calc:
	_cs->calc_u8 = _gga->string[1];
	for (int i=2; i<(_gga->length - 5); i++)
	{
		_cs->calc_u8 ^= _gga->string[i];
	}

	//	compare:
	if (_cs->calc_u8 == _cs->glue_u8)
	{
		_cs->status_flag = 1;
		_flag->correct_packet_cnt_u32++;
		return 1;
	}
	return 0;
}
//***********************************************************

void ShutDown(void)
{
#if (NAUR_FI_F446 == 1)
	LCD_FillScreen(ILI92_BLACK);
#elif (NAUR_FI_F103 == 1)
	LCD_FillScreen(ILI92_WHITE);
#endif

	LCD_SetCursor(0, 0);
	for (int i=4; i>=0; i--)
	{
		sprintf(DebugString,"SHUTDOWN %d\r\n", i);
		HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
		LCD_Printf("%s",DebugString);
		HAL_GPIO_WritePin(BEEPER_GPIO_Port, BEEPER_Pin, GPIO_PIN_RESET);
		HAL_Delay(800);
		HAL_GPIO_WritePin(BEEPER_GPIO_Port, BEEPER_Pin, GPIO_PIN_SET);
		HAL_Delay(300);
		//HAL_IWDG_Refresh(&hiwdg);
	}
#if (NAUR_FI_F446 == 1)
	LCD_FillScreen(ILI92_BLACK);
#elif (NAUR_FI_F103 == 1)
	LCD_FillScreen(ILI92_WHITE);
#endif

	LCD_SetCursor(0, 0);
}
//***********************************************************

void Write_SD_card(GGA_struct * _gga, SD_Card_struct * _sd)
{
#if (NAUR_FI_F446 == 1)
	fres = f_open(&USERFile, _sd->filename, FA_OPEN_APPEND | FA_WRITE );			/* Try to open file */
#elif (NAUR_FI_F103 == 1)
	fres = f_open(&USERFile, _sd->filename, FA_OPEN_ALWAYS | FA_WRITE);
	fres += f_lseek(&USERFile, f_size(&USERFile));
#endif
	_sd->write_status = fres;
	if (fres == FR_OK)
	{
		HAL_IWDG_Refresh(&hiwdg);
		f_printf(&USERFile, "%s", _gga->string);	/* Write to file */
		_sd->file_size = f_size(&USERFile);
		f_close(&USERFile);	/* Close file */
	}
	else
	{
		#if (DEBUG_MODE == 1)
			HAL_IWDG_Refresh(&hiwdg);
		#else
			Beep();
		#endif
	}
}
//***********************************************************

void Beep (void)
{
	HAL_GPIO_WritePin(BEEPER_GPIO_Port, BEEPER_Pin, GPIO_PIN_RESET);
	HAL_Delay(50);
	HAL_GPIO_WritePin(BEEPER_GPIO_Port, BEEPER_Pin, GPIO_PIN_SET);
}
//***********************************************************

void Copy_GGA_Force(NEO6_struct * _neo6, GGA_struct * _gga)
{
	char tmp_str[GGA_FORCE_LENGTH];
	memset(tmp_str, 0, GGA_FORCE_LENGTH);
	//memcpy(_gga->string, &_neo6->string[GGA_FORCE_START], GGA_FORCE_LENGTH    );
	       memcpy(tmp_str, &_neo6->string[GGA_FORCE_START], GGA_FORCE_LENGTH - 2);
	snprintf(_gga->string, GGA_FORCE_LENGTH + 1,"%s\r\n", tmp_str);
}
//***********************************************************

void Copy_GGA_Correct(NEO6_struct * _neo6, GGA_struct * _gga)
{
	char tmp_str[GGA_STRING_MAX_SIZE];
	memset(tmp_str, 0, GGA_STRING_MAX_SIZE);
	//memcpy(_gga->string, &_neo6->string[_gga->Neo6_start], _gga->length    );
	       memcpy(tmp_str, &_neo6->string[_gga->Neo6_start], _gga->length - 2);
	snprintf(_gga->string, _gga->length + 1 ,"%s\r\n", tmp_str);

}
//***********************************************************

void Clear_variables(NEO6_struct * _neo6, GGA_struct * _gga, CheckSum_struct * _cs, Flags_struct * _flag, SD_Card_struct * _sd)
{
	_gga->Neo6_start 		= 0;
	_gga->Neo6_end   		= 0;
	_gga->beginning_chars	= 0;
	_gga->ending_char		= 0;
	_gga->length = 0;

	_cs->calc_u8 		= 0;
	_cs->glue_u8 		= 0;
	_cs->status_flag 	= 0;

	_sd->write_status	= 0;

	_neo6->length_int = 0;
	_flag->no_signal = 0;
}
//***********************************************************

void Read_SD_card(void)
{
	sprintf(DebugString,"Start read SD\r\n");
	HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);

	fres = f_open(&USERFile, "tmm.txt", FA_OPEN_EXISTING | FA_READ);
	if (fres == FR_OK)
	{
		sprintf(DebugString,"st: \r\n");
		HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);

			char buff[200];
			LCD_SetCursor(0, 0);
			LCD_FillScreen(0x0000);
			/* Read from file */
			while (f_gets(buff, 200, &USERFile))
			{
				LCD_Printf(buff);
				sprintf(DebugString,"%s\r\n", buff);
				HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
			}
		f_close(&USERFile);	/* Close file */
	}
	else
	{
		sprintf(DebugString,"6) read from SD FAILED\r\n");
		HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
	}

	sprintf(DebugString,"7) END read from SD.\r\n");
	HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
}
//***********************************************************

void Prepare_filename(CheckSum_struct * _cs, Time_struct * _time, SD_Card_struct * _sd)
{
	if (_sd->file_size > (CLUSTER_SIZE - GGA_STRING_MAX_SIZE ) )
	{
		_sd->file_name_int = _time->hour_int*10000 + _time->minutes_int*100 + _time->seconds_int ;
	}

	char file_name_char[FILE_NAME_SIZE];
	sprintf(file_name_char,"%06d_%d.txt", _sd->file_name_int, (int)_cs->status_flag);
	int len = strlen(file_name_char) + 1;
	char PathString[len];
	snprintf(PathString, len,"%s", file_name_char);

	TCHAR *f_tmp = _sd->filename;
	char *s_tmp = PathString;
	while(*s_tmp)
	 *f_tmp++ = (TCHAR)*s_tmp++;
	*f_tmp = 0;
}
//***********************************************************

void Read_from_RingBuffer(NEO6_struct * _neo6, RingBuffer_DMA * _rx_buffer, Flags_struct * _flag)
{
  	uint32_t rx_count = RingBuffer_DMA_Count(_rx_buffer);
	while ((rx_count--) && (_flag->end_of_UART_packet == 0))
	{
		_neo6->string[_neo6->length_int] = RingBuffer_DMA_GetByte(_rx_buffer);
		_neo6->length_int++;
		if (_neo6->length_int > MAX_CHAR_IN_NEO6)
		{
			_flag->end_of_UART_packet = 1;
			return;
		}
	}
}
//***********************************************************

void Update_flag_shudown_button_pressed(void)
{
	FLAG.shudown_button_pressed	= 1 ;
}
//***********************************************************

void Update_flag_end_of_UART_packet(void)
{
	FLAG.end_of_UART_packet = 1;
}
//***********************************************************

void Update_No_Signal(void)
{
	FLAG.no_signal = 1;
}
//***********************************************************

void Print_No_signal(Flags_struct * _flag)
{
	sprintf(DebugString,"%d)NO signal from GPS                                       \r\n", _flag->no_signal_cnt);
	HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
	LCD_SetCursor(0, 95*(_flag->no_signal_cnt%2));
	LCD_Printf("%s", DebugString);
	_flag->no_signal_cnt++;
}
//***********************************************************
//***********************************************************
//***********************************************************
