
#include "main.h"
#include "NAURocket_FindIt_sm.h"

//***********************************************************

	extern DMA_HandleTypeDef		hdma_usart2_rx;
	extern DMA_HandleTypeDef		hdma_usart3_rx;

	uint8_t rx_circular_buffer[GPS_CH_QNT][RX_BUFFER_SIZE];

//***********************************************************
//***********************************************************

	void Clear_GPS_struct		(GPS_struct 	* _gps);
	void Clear_SD_Card_struct	(SD_Card_struct * _sd );

	void Read_from_RingBuffer(GPS_struct * _gps, RingBuffer_DMA * buffer);

	void Prepare_filename(SD_Card_struct * _sd);
	void SDcard_Write(GPS_struct * _gps, SD_Card_struct * _sd);

	void Print_GPS_to_UART(GPS_struct * _gps) ;
	void Print_GPS_to_LCD(GPS_struct * _gps) ;
	void Print_SD_Card_to_UART(SD_Card_struct * _sd ) ;
	void Print_No_signal(GPS_channel _channel);

	void TIM_time_overflow_start(GPS_channel _channel);
	void TIM_time_overflow_stop(GPS_channel _channel);
	void TIM_time_overflow_reset(GPS_channel _channel);

	void RTU_start(GPS_channel _channel);
	void RTU_stop(GPS_channel _channel);

	void TIM_no_signal_Start (GPS_channel _channel);
	void TIM_no_signal_Stop  (GPS_channel _channel);
	void TIM_no_signal_Reset (GPS_channel _channel);

	void Beep(void);
	void ShutDown(void);

	void Check_bytes_in_UART_packet (GPS_channel _channel) ;

	uint8_t Find_Begin_of_GGA_string(GPS_struct * _gps, GGA_struct* _gga )							;
	uint8_t Find_End_of_GGA_string	(GPS_struct * _gps, GGA_struct* _gga ) 							;
	void Copy_GGA_Force				(GPS_struct * _gps, GGA_struct* _gga )							;
	void Copy_GGA_Correct			(GPS_struct * _gps, GGA_struct* _gga ) 							;
	uint8_t Calc_SheckSum_GGA		(GPS_struct * _gps, GGA_struct* _gga, CheckSum_struct * _cs )	;
	void Get_time_from_GGA_string	(GGA_struct * _gga, Time_struct * _time, SD_Card_struct * _sd)	;

	void Parse_GGA_string			(GGA_struct * _gga, GPS_struct * _gps, Coordinates_struct * _coord)		;
	void Print_Coord_DMM_to_UART	(Coordinates_struct * _coord, GPS_struct * _gps) 					;

	void Convert_from_DMM_to_DD		(Coordinates_struct * _coord ) ;
	void Print_Coord_DD_to_UART		(Coordinates_struct * _coord, GPS_struct * _gps) 					;

	void Calc_rocket_direction (Coordinates_struct * _coord_base, Coordinates_struct * _coord_rocket, Rocket_struct * _rocket) ;

//***********************************************************
//***********************************************************

void NAUR_Init (void){
	HAL_StatusTypeDef	status_res;
	char DebugString[DEBUG_STRING_SIZE];
	LCD_Init();
	LCD_SetRotation(1);
	LCD_SetCursor(0, 0);
	LCD_FillScreen(ILI92_BLACK);
	LCD_SetTextColor(ILI92_GREEN, ILI92_BLACK);

	Debug_ch.uart = &huart5;

	GPS[2].channel = GPS_CH_2;
	GPS[2].rtu_handler = &htim2;
	GPS[2].tim_no_sigmal_handler = &htim4;
	GPS[2].time_overflow_handler = &htim5;

	GPS[3].channel = GPS_CH_3;
	GPS[3].rtu_handler = &htim3;
	GPS[3].tim_no_sigmal_handler = &htim6;
	GPS[3].time_overflow_handler = &htim7;

	sprintf(DebugString,"\r\n\r\n");
	HAL_UART_Transmit(Debug_ch.uart, (uint8_t *)DebugString, strlen(DebugString), 100);

	int soft_version_arr_int[3];
	soft_version_arr_int[0] = ((SOFT_VERSION) / 100) %10 ;
	soft_version_arr_int[1] = ((SOFT_VERSION) /  10) %10 ;
	soft_version_arr_int[2] = ((SOFT_VERSION)      ) %10 ;
	sprintf(DebugString,"NAUR Find It F446\r\n2020-April-21 v%d.%d.%d \r\nfor_debug UART5 115200/8-N-1\r\n",
			soft_version_arr_int[0], soft_version_arr_int[1], soft_version_arr_int[2]);
	HAL_UART_Transmit(Debug_ch.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
	LCD_Printf("%s",DebugString);

	RingBuffer_DMA_Init(&rx_buffer[3], &hdma_usart3_rx, rx_circular_buffer[3], RX_BUFFER_SIZE);  	// Start UART receive
	status_res = HAL_UART_Receive_DMA(&huart3, rx_circular_buffer[3], RX_BUFFER_SIZE);  	// how many bytes in buffer
	sprintf(DebugString, "ch3: UART_3_DMA status:%d\r\n", (int)status_res);
	HAL_UART_Transmit(Debug_ch.uart, (uint8_t *)DebugString, strlen(DebugString), 100);

	RingBuffer_DMA_Init(&rx_buffer[2], &hdma_usart2_rx, rx_circular_buffer[2], RX_BUFFER_SIZE);  	// Start UART receive
	status_res = HAL_UART_Receive_DMA(&huart2, rx_circular_buffer[2], RX_BUFFER_SIZE);  	// how many bytes in buffer
	sprintf(DebugString, "ch2: UART_2_DMA status:%d\r\n", (int)status_res);
	HAL_UART_Transmit(Debug_ch.uart, (uint8_t *)DebugString, strlen(DebugString), 100);

	FATFS_SPI_Init(&hspi1);	/* Initialize SD Card low level SPI driver */
	static uint8_t try_u8;
	do{
		fres = f_mount(&USERFatFS, "", 1);	/* try to mount SDCARD */
		if (fres == FR_OK) {
			sprintf(DebugString,"\r\nSDcard mount - Ok \r\n");
			HAL_UART_Transmit(Debug_ch.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
			LCD_Printf("%s",DebugString);
		} else {
			f_mount(NULL, "", 0);			/* Unmount SDCARD */
			Error_Handler();
			try_u8++;
			sprintf(DebugString,"%d)SDcard mount: Failed  Error: %d\r\n", try_u8, fres);
			HAL_UART_Transmit(Debug_ch.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
			LCD_Printf("%s",DebugString);
			Beep();
			HAL_Delay(1000);
//			if (try_u8 == 3)
//			{
//				LCD_SetCursor(0, 0);
//				#if (NAUR_FI_F446 == 1)
//					LCD_FillScreen(ILI92_BLACK);
//				#elif (NAUR_FI_F103 == 1)
//					LCD_FillScreen(ILI92_WHITE);
//				#endif
//			}
		}
	}
	#if (DEBUG_MODE == 1)
		while (0);
	#else
		while ((fres !=0) && (try_u8 < 5));
	#endif
	//***********************************************************

	if (hdma_usart3_rx.State == HAL_DMA_STATE_ERROR) {
			HAL_UART_DMAStop(&huart3);
			status_res = HAL_UART_Receive_DMA(&huart3, rx_circular_buffer[3], RX_BUFFER_SIZE);
			sprintf(DebugString, "ch3: UART_3_DMA status:%d\r\n", (int)status_res);
	} else {
		sprintf(DebugString, "ch3: UART_3_DMA status - Ok.\r\n");
	}
	HAL_UART_Transmit(Debug_ch.uart, (uint8_t *)DebugString, strlen(DebugString), 100);

	if (hdma_usart2_rx.State == HAL_DMA_STATE_ERROR) {
			HAL_UART_DMAStop(&huart2);
			status_res = HAL_UART_Receive_DMA(&huart2, rx_circular_buffer[2], RX_BUFFER_SIZE);
			sprintf(DebugString, "ch2: UART_2_DMA status:%d\r\n", (int)status_res);
	} else {
		sprintf(DebugString, "ch2: UART_2_DMA status - Ok.\r\n");
	}
	HAL_UART_Transmit(Debug_ch.uart, (uint8_t *)DebugString, strlen(DebugString), 100);

	//***********************************************************

	LCD_FillScreen(ILI92_BLACK);

	TIM_time_overflow_start(GPS_CH_2);
	TIM_time_overflow_start(GPS_CH_3);

	TIM_no_signal_Start(GPS_CH_2);
	TIM_no_signal_Start(GPS_CH_3);

	HAL_IWDG_Refresh(&hiwdg);
}

//***********************************************************
//***********************************************************

void NAUR_Main (void) {
	switch (sm_stage) {
		case SM_START: {
			Clear_GPS_struct(&GPS[2]);
			Clear_GPS_struct(&GPS[3]);
			Clear_SD_Card_struct(&SD);

			RTU_reset(GPS_CH_2);
			RTU_start(GPS_CH_2);

			RTU_reset(GPS_CH_3);
			RTU_start(GPS_CH_3);

			TIM_time_overflow_reset(GPS_CH_2);
			TIM_time_overflow_reset(GPS_CH_3);

			sm_stage =SM_READ_FROM_RINGBUFFER;
		} break;

	//--------------------------------------------------------

		case SM_READ_FROM_RINGBUFFER: {
			Read_from_RingBuffer(&GPS[3], &rx_buffer[3] );
			Read_from_RingBuffer(&GPS[2], &rx_buffer[2] );
			sm_stage = SM_CHECK_FLAGS;
		} break;

	//--------------------------------------------------------

		case SM_CHECK_FLAGS: {
			Check_bytes_in_UART_packet(GPS_CH_2);
			Check_bytes_in_UART_packet(GPS_CH_3);

			if ((GPS[2].UART_packet_ready_flag == FLAG_SET) && (GPS[3].UART_packet_ready_flag == FLAG_SET)) {
				sm_stage = SM_FIND_GGA;
				break;
			}

			if ((GPS[GPS_CH_2].packet_overflow_flag == FLAG_SET) || (GPS[GPS_CH_2].time_overflow_flag == FLAG_SET)) {
				RTU_stop(GPS_CH_2);
				Beep();
				sm_stage = SM_FIND_GGA;
				break;
			}

			if ((GPS[GPS_CH_3].packet_overflow_flag == FLAG_SET) || (GPS[GPS_CH_3].time_overflow_flag == FLAG_SET)) {
				RTU_stop(GPS_CH_3);
				Beep();
				sm_stage = SM_FIND_GGA;
				break;
			}

			if (SD.shudown_button_pressed_flag == FLAG_SET) {
				ShutDown();
				sm_stage = SM_FINISH;	//	but it must Reset by IWDT
				break;
			}

			if (GPS[GPS_CH_2].no_signal_flag == FLAG_SET) {
				Print_No_signal(GPS_CH_2);
				sm_stage = SM_FINISH;
				break;
			}

			if (GPS[GPS_CH_3].no_signal_flag == FLAG_SET) {
				Print_No_signal(GPS_CH_3);
				sm_stage = SM_FINISH;
				break;
			}

			sm_stage = SM_READ_FROM_RINGBUFFER;
		} break;
	//--------------------------------------------------------

		case SM_FIND_GGA:		{
			result = Find_Begin_of_GGA_string(&GPS[GPS_CH_2], &GGA[GPS_CH_2]);	/// tut kasha
			result = Find_Begin_of_GGA_string(&GPS[GPS_CH_3], &GGA[GPS_CH_3]);

			if ( result == R_OK)
			{
				sm_stage = SM_FIND_ASTERISK;
			}
			else
			{
				Copy_GGA_Force(&GPS[GPS_CH_2], &GGA[GPS_CH_2]);
				Copy_GGA_Force(&GPS[GPS_CH_3], &GGA[GPS_CH_3]);
				sm_stage = SM_PREPARE_FILENAME;
			}
		} break;
		//--------------------------------------------------------

		case SM_FIND_ASTERISK:	{
			result = Find_End_of_GGA_string(&GPS[GPS_CH_2], &GGA[GPS_CH_2]);
			result = Find_End_of_GGA_string(&GPS[GPS_CH_3], &GGA[GPS_CH_3]);
			if ( result == R_OK ) {
				Copy_GGA_Correct(&GPS[GPS_CH_2], &GGA[GPS_CH_2]);
				Copy_GGA_Correct(&GPS[GPS_CH_3], &GGA[GPS_CH_3]);
				sm_stage = SM_CALC_SHECKSUM;
			} else {
				Copy_GGA_Force(&GPS[GPS_CH_2], &GGA[GPS_CH_2]);
				Copy_GGA_Force(&GPS[GPS_CH_3], &GGA[GPS_CH_3]);
				sm_stage = SM_PREPARE_FILENAME;
			}
		} break;
		//--------------------------------------------------------

		case SM_CALC_SHECKSUM:	{
			result = Calc_SheckSum_GGA(&GPS[GPS_CH_2], &GGA[GPS_CH_2], &CS[GPS_CH_2]);
			result = Calc_SheckSum_GGA(&GPS[GPS_CH_3], &GGA[GPS_CH_3], &CS[GPS_CH_3]);
			if ( result == R_OK )
			{
				Get_time_from_GGA_string(&GGA[GPS_CH_2], &Time[GPS_CH_2], &SD);
				Get_time_from_GGA_string(&GGA[GPS_CH_3], &Time[GPS_CH_3], &SD);

				Parse_GGA_string(&GGA[GPS_CH_2], &GPS[GPS_CH_2], &Coord[GPS_CH_2]);
				Parse_GGA_string(&GGA[GPS_CH_3], &GPS[GPS_CH_3], &Coord[GPS_CH_3]);
			}
			sm_stage = SM_PREPARE_FILENAME;
		} break;
		//--------------------------------------------------------


		case SM_PREPARE_FILENAME: {
			Prepare_filename(&SD);
			sm_stage = SM_WRITE_SDCARD;
		} break;
	//--------------------------------------------------------

		case SM_WRITE_SDCARD: {
			SDcard_Write(&GPS[3], &SD);
			sm_stage = SM_PRINT_ALL_INFO;
		} break;
	//--------------------------------------------------------

		case SM_PRINT_ALL_INFO: {
			Print_GPS_to_UART(&GPS[2]) ;
			Print_GPS_to_UART(&GPS[3]) ;

//			Print_Coord_DMM_to_UART(&Coord[GPS_CH_2], &GPS[GPS_CH_2]);
//			Print_Coord_DMM_to_UART(&Coord[GPS_CH_3], &GPS[GPS_CH_3]);

			Convert_from_DMM_to_DD(&Coord[GPS_CH_2]) ;
			Convert_from_DMM_to_DD(&Coord[GPS_CH_3]) ;

			Print_Coord_DD_to_UART(&Coord[GPS_CH_2], &GPS[GPS_CH_2]);
			Print_Coord_DD_to_UART(&Coord[GPS_CH_3], &GPS[GPS_CH_3]);

			Calc_rocket_direction (&Coord[GPS_CH_2], &Coord[GPS_CH_3], &Rocket);

			int delta_int = GPS[2].sys_tick_u32 - GPS[3].sys_tick_u32;
			char DebugString[DEBUG_STRING_SIZE];
			sprintf(DebugString, "delta_sys_tick:%d\t", delta_int);
			HAL_UART_Transmit(Debug_ch.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
			//	if ((delta_int< -500) || (delta_int > 500))  HAL_Delay(500);
			Print_SD_Card_to_UART(&SD) ;

			LCD_FillScreen(ILI92_BLACK);
			LCD_SetCursor(0, 0);
			Print_GPS_to_LCD(&GPS[2]);
			Print_GPS_to_LCD(&GPS[3]);

			sm_stage = SM_FINISH;
		} break;
	//--------------------------------------------------------

		case SM_FINISH: {
			sm_stage = SM_START;
		} break;
	//--------------------------------------------------------

		default: {
			sm_stage = SM_START;
		} break;
	}	//			switch (sm_stage)
}
//***********************************************************

void Check_bytes_in_UART_packet (GPS_channel _channel) {
	if (GPS[_channel].end_of_UART_packet_flag == FLAG_SET) {
		GPS[_channel].end_of_UART_packet_flag = FLAG_RESET;
		if ((GPS[_channel].length_int > 0) && (GPS[_channel].UART_packet_ready_flag == FLAG_RESET)) {
			RTU_stop(_channel);
			TIM_no_signal_Reset(_channel);
			GPS[_channel].UART_packet_ready_flag = FLAG_SET;
			GPS[_channel].sys_tick_u32 = HAL_GetTick();
		}
	}
}
//***********************************************************

void Print_GPS_to_LCD(GPS_struct * _gps) {
	char DebugString[DEBUG_STRING_SIZE];
	//	LCD_SetCursor(0, 95*(_time->seconds_int%2));
	uint8_t lcd_circle = _gps->length_int / 254;
	char tmp_str[0xFF];
	for (int i = 0; i < lcd_circle; i++) {
		memcpy(tmp_str, &_gps->string[i*254], 255-1);
		snprintf(DebugString, 255,"%s", tmp_str);
		LCD_Printf("%s", DebugString);
	}
	uint8_t tmp_ctr_size = _gps->length_int - 254 * lcd_circle;

	memcpy(tmp_str, &_gps->string[lcd_circle*254], tmp_ctr_size);
	snprintf(DebugString, tmp_ctr_size + 1, "%s", tmp_str);
	LCD_Printf("%s\r\n", DebugString);
}
//***********************************************************

void Print_GPS_to_UART(GPS_struct * _gps) {
	char DebugString[DEBUG_STRING_SIZE];
	snprintf(DebugString, _gps->length_int + 4,"%d) %s", (int)_gps->channel, _gps->string);
	HAL_UART_Transmit(Debug_ch.uart, (uint8_t *)DebugString, strlen(DebugString), 100);

	if (_gps->time_overflow_flag == FLAG_SET) {
		sprintf(DebugString,">> ## ch%d Time_overflow ##\r\n", (int)_gps->channel);
		HAL_UART_Transmit(Debug_ch.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
	}

	if (_gps->packet_overflow_flag == FLAG_SET) {
		sprintf(DebugString,">> ## ch%d Packet_overflow ##\r\n", (int)_gps->channel);
		HAL_UART_Transmit(Debug_ch.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
	}
}
//***********************************************************

void Print_Coord_DMM_to_UART(Coordinates_struct * _coord, GPS_struct * _gps) {
	char DebugString[DEBUG_STRING_SIZE];
	sprintf(DebugString,"ch%d\tLat:%4.5f\tLon:%5.5f\tAlt:%3.1f\tsat:%d\r\n",
		(int)_gps->channel,
		_coord->latitude_DMM_dbl,
		_coord->longitude_DMM_dbl,
		_coord->altitude_dbl,
		_coord->satelite_qnt_int);
	HAL_UART_Transmit(Debug_ch.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
}
//***********************************************************

void Print_Coord_DD_to_UART(Coordinates_struct * _coord, GPS_struct * _gps) {
	char DebugString[DEBUG_STRING_SIZE];
	sprintf(DebugString,"ch%d\tLat:%f\tLon:%f\tAlt:%3.1f\tsat:%d\r\n",
		(int)_gps->channel,
		_coord->latitude_DD_dbl,
		_coord->longitude_DD_dbl,
		_coord->altitude_dbl,
		_coord->satelite_qnt_int);
	HAL_UART_Transmit(Debug_ch.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
}
//***********************************************************

void Print_SD_Card_to_UART(SD_Card_struct * _sd ) {
	char DebugString[DEBUG_STRING_SIZE];
	sprintf(DebugString,">> file_name:%s; size: %d; SD_write: %d\r\n\r\n",
		_sd->file_name_char,
		(int)_sd->file_size,
		_sd->write_status_fr);
	HAL_UART_Transmit(Debug_ch.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
}
//***********************************************************

void TIM_time_overflow_start(GPS_channel _channel) {
	HAL_TIM_Base_Start_IT(GPS[_channel].time_overflow_handler);
	HAL_TIM_Base_Start   (GPS[_channel].time_overflow_handler);
}
//***********************************************************

void TIM_time_overflow_stop(GPS_channel _channel) {
	HAL_TIM_Base_Stop_IT(GPS[_channel].time_overflow_handler);
	HAL_TIM_Base_Stop   (GPS[_channel].time_overflow_handler);
}
//***********************************************************

void TIM_time_overflow_reset(GPS_channel _channel) {
	switch (_channel) {
		case GPS_CH_0: 						break;
		case GPS_CH_1: 						break;
		case GPS_CH_2: 		TIM5->CNT = 0;	break;
		case GPS_CH_3:		TIM7->CNT = 0;	break;
		case GPS_CH_QNT:					break;
		default: 							break;
	}
}
//***********************************************************

void RTU_start(GPS_channel _channel) {
	HAL_TIM_Base_Start_IT(GPS[_channel].rtu_handler);
	HAL_TIM_Base_Start   (GPS[_channel].rtu_handler);
}
//***********************************************************

void RTU_stop(GPS_channel _channel) {
	HAL_TIM_Base_Stop_IT(GPS[_channel].rtu_handler);
	HAL_TIM_Base_Stop   (GPS[_channel].rtu_handler);
}
//***********************************************************

void RTU_reset(GPS_channel _channel) {
	switch (_channel) {
		case GPS_CH_0: 						break;
		case GPS_CH_1: 						break;
		case GPS_CH_2: 		TIM2->CNT = 0;	break;
		case GPS_CH_3:		TIM3->CNT = 0;	break;
		case GPS_CH_QNT:					break;
		default: 							break;
	}
	TIM_no_signal_Reset(_channel);
}
//***********************************************************

void TIM_no_signal_Start(GPS_channel _channel) {
	HAL_TIM_Base_Start_IT(GPS[_channel].tim_no_sigmal_handler);
	HAL_TIM_Base_Start   (GPS[_channel].tim_no_sigmal_handler);
}
//***********************************************************

void TIM_no_signal_Stop(GPS_channel _channel) {
	HAL_TIM_Base_Stop_IT(GPS[_channel].tim_no_sigmal_handler);
	HAL_TIM_Base_Stop   (GPS[_channel].tim_no_sigmal_handler);
}
//***********************************************************

void TIM_no_signal_Reset(GPS_channel _channel) {
	switch (_channel) {
		case GPS_CH_0: 						break;
		case GPS_CH_1: 						break;
		case GPS_CH_2: 		TIM4->CNT = 0;	break;
		case GPS_CH_3:		TIM6->CNT = 0;	break;
		case GPS_CH_QNT:					break;
		default: 							break;
	}
}
//***********************************************************

void ShutDown(void) {
	char DebugString[DEBUG_STRING_SIZE];
	LCD_FillScreen(ILI92_BLACK);
	LCD_SetCursor(0, 0);
	for (int i=5; i>=0; i--) {
		sprintf(DebugString,"Remove SD-card! Left %d sec.\r\n", i);
		HAL_UART_Transmit(Debug_ch.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
		LCD_Printf("%s",DebugString);
		HAL_GPIO_WritePin(BEEPER_GPIO_Port, BEEPER_Pin, GPIO_PIN_SET);
		HAL_Delay(1000);
		HAL_GPIO_WritePin(BEEPER_GPIO_Port, BEEPER_Pin, GPIO_PIN_RESET);
		HAL_Delay(400);
	}
	LCD_FillScreen(ILI92_BLACK);
	LCD_SetCursor(0, 0);
}
//***********************************************************

void SDcard_Write(GPS_struct * _gps, SD_Card_struct * _sd) {
	char DebugString[DEBUG_STRING_SIZE];
	snprintf(DebugString, _gps->length_int+1,"%s", _gps->string);
#if (NAUR_FI_F446 == 1)
	fres = f_open(&USERFile, _sd->file_name_char, FA_OPEN_APPEND | FA_WRITE );			/* Try to open file */
#elif (NAUR_FI_F103 == 1)
	fres = f_open(&USERFile, _sd->file_name_char, FA_OPEN_ALWAYS | FA_WRITE);
	fres += f_lseek(&USERFile, f_size(&USERFile));
#endif
	_sd->write_status_fr = fres;
	if (fres == FR_OK) {
		HAL_IWDG_Refresh(&hiwdg);
		f_printf(&USERFile, "%s", DebugString);	/* Write to file */
		_sd->file_size = f_size(&USERFile);
		f_close(&USERFile);	/* Close file */
	} else {
		#if (DEBUG_MODE == 1)
			HAL_IWDG_Refresh(&hiwdg);
		#else
			Beep();
		#endif
	}
}
//***********************************************************

void Beep (void) {
	HAL_GPIO_WritePin(BEEPER_GPIO_Port, BEEPER_Pin, GPIO_PIN_RESET);
	HAL_Delay(50);
	HAL_GPIO_WritePin(BEEPER_GPIO_Port, BEEPER_Pin, GPIO_PIN_SET);
}
//***********************************************************

void Clear_GPS_struct(GPS_struct * _gps) {
	_gps->length_int				= FLAG_RESET ;
	_gps->end_of_UART_packet_flag	= FLAG_RESET ;
	_gps->no_signal_flag	 		= FLAG_RESET ;
	_gps->time_overflow_flag		= FLAG_RESET ;
	_gps->packet_overflow_flag		= FLAG_RESET ;
	_gps->UART_packet_ready_flag	= FLAG_RESET ;
}
//***********************************************************

void Clear_SD_Card_struct(SD_Card_struct * _sd) {
	_sd->write_status_fr			= FR_OK ;
}
//***********************************************************

void Prepare_filename(SD_Card_struct * _sd) {
	_sd->file_name_int++;
	char file_name_char[FILE_NAME_SIZE];
	sprintf(file_name_char,"%06d.txt", _sd->file_name_int);
	int len = strlen(file_name_char) + 1;
	char PathString[len];
	snprintf(PathString, len,"%s", file_name_char);

	TCHAR *f_tmp = _sd->file_name_char;
	char *s_tmp = PathString;
	while(*s_tmp)
	 *f_tmp++ = (TCHAR)*s_tmp++;
	*f_tmp = 0;
}
//***********************************************************

void Read_from_RingBuffer(GPS_struct * _gps, RingBuffer_DMA * _rx_buffer) {
  	uint32_t rx_count = RingBuffer_DMA_Count(_rx_buffer);
	while (rx_count--) {
		_gps->string[_gps->length_int] = RingBuffer_DMA_GetByte(_rx_buffer);
		_gps->length_int++;
		if (_gps->length_int > MAX_CHAR_IN_GPS) {
			_gps->packet_overflow_flag = FLAG_SET;
			return;
		}
	}
}
//***********************************************************

void Set_flag_Shudown_button_pressed(void) {
	SD.shudown_button_pressed_flag	= FLAG_SET ;
}
//***********************************************************

void Set_flag_End_of_UART_packet(GPS_channel _channel) {
	GPS[_channel].end_of_UART_packet_flag = FLAG_SET;
}
//***********************************************************

void Set_flag_No_signal(GPS_channel _channel) {
	GPS[_channel].no_signal_flag = FLAG_SET;
}
//***********************************************************

void Set_flag_time_overflow_package(GPS_channel _channel) {
	GPS[_channel].time_overflow_flag = FLAG_SET;
}
//***********************************************************

void Print_No_signal(GPS_channel _channel) {
	char DebugString[DEBUG_STRING_SIZE];
	sprintf(DebugString,">> ## NO signal from GPS %d ##\r\n", (int)_channel);
	HAL_UART_Transmit(Debug_ch.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
	LCD_FillScreen(ILI92_BLACK);
	LCD_SetCursor(0, 0);
	LCD_Printf("%s", DebugString);
	Beep();
}
//***********************************************************

uint8_t Find_Begin_of_GGA_string(GPS_struct * _gps, GGA_struct* _gga ) {
	for (int i=2; i<= _gps->length_int; i++) {
		if (memcmp(&_gps->string[i-2], "GGA" ,3) == 0) {
			_gga->Neo6_start = i - 5;	//	$GPGGA
			_gga->beginning_chars_present = 1;	//	012345
//			char DebugString[DEBUG_STRING_SIZE];
//			sprintf(DebugString,"ch%d: find begin_of_GGA - Ok. \r\n", (int)_gps->channel);
//			HAL_UART_Transmit(Debug_ch.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
			return 1;
		}
	}
	return 0;
}
//***********************************************************

void Copy_GGA_Force(GPS_struct * _gps, GGA_struct* _gga ) {
	char tmp_str[GGA_FORCE_LENGTH];
	memset(tmp_str, 0, GGA_FORCE_LENGTH);
	//memcpy(_gga->string, &_neo6->string[GGA_FORCE_START], GGA_FORCE_LENGTH    );
	       memcpy(tmp_str, &_gps->string[GGA_FORCE_START], GGA_FORCE_LENGTH - 2);
	snprintf(_gga->string, GGA_FORCE_LENGTH + 1,"%s\r\n", tmp_str);
}
//***********************************************************


uint8_t Find_End_of_GGA_string(GPS_struct * _gps, GGA_struct* _gga ) {
	if (_gga->beginning_chars_present == 0)	{
		return 0;
	}

	for (int i=_gga->Neo6_start; i<=_gps->length_int; i++) {
		if (_gps->string[i] == '*') {
			_gga->Neo6_end = i + 5;
			_gga->length = _gga->Neo6_end - _gga->Neo6_start;
			if ((_gga->length < GGA_LENGTH_MIN) || (_gga->length > GGA_STRING_MAX_SIZE)) {
				return 0;
			}
			_gga->ending_char_present = 1;
//			char DebugString[DEBUG_STRING_SIZE];
//			sprintf(DebugString,"ch%d: find end_of_GGA - Ok. \r\n", (int)_gps->channel);
//			HAL_UART_Transmit(Debug_ch.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
			return 1;
		}
	}
	return 0;
}
//***********************************************************

void Copy_GGA_Correct(GPS_struct * _gps, GGA_struct* _gga ) {
	char tmp_str[GGA_STRING_MAX_SIZE];
	memset(tmp_str, 0, GGA_STRING_MAX_SIZE);
	//memcpy(_gga->string, &_neo6->string[_gga->Neo6_start], _gga->length    );
	       memcpy(tmp_str, &_gps->string[_gga->Neo6_start], _gga->length - 2);
	snprintf(_gga->string, _gga->length + 1 ,"%s\r\n", tmp_str);
}
//***********************************************************

uint8_t Calc_SheckSum_GGA(GPS_struct * _gps, GGA_struct * _gga, CheckSum_struct * _cs ) {
	#define CS_SIZE	4
	char cs_glue_string[CS_SIZE];

	/*	glue	*/
	memset(cs_glue_string, 0, CS_SIZE);
	memcpy(cs_glue_string, &_gga->string[_gga->length - 4], 2);
	_cs->glue_u8 = strtol(cs_glue_string, NULL, 16);

	/*	calc	*/
	_cs->calc_u8 = _gga->string[1];
	for (int i=2; i<(_gga->length - 5); i++) {
		_cs->calc_u8 ^= _gga->string[i];
	}

	/*	compare	*/
	if (_cs->calc_u8 == _cs->glue_u8) {
		_cs->status_flag = 1;
		return 1;
	}
	return 0;
}
//***********************************************************

void Get_time_from_GGA_string(GGA_struct * _gga, Time_struct * _time, SD_Card_struct * _sd) {
	if (_time->updated_flag == 1) {
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

void Parse_GGA_string(GGA_struct * _gga, GPS_struct * _gps, Coordinates_struct * _coord) {
	uint8_t pole_u8 = 0;
	#define	GPS_POLE_SIZE	15
	#define	GPS_POLE_QNT	15
	char gps_string[GPS_POLE_QNT][GPS_POLE_SIZE];

	for (int i=0; i < GPS_POLE_QNT; i++) {
		memset(gps_string[i],0,GPS_POLE_SIZE);
	}

	uint8_t previous_pole_position_u8 = 0;
	for (int position = _gga->Neo6_start; position < _gga->Neo6_end; position++) {
		if (_gga->string[position] == ',') {
			memcpy(gps_string[pole_u8], &_gga->string[previous_pole_position_u8], (position - previous_pole_position_u8));
//			sprintf(DebugString,"%d)%s ",pole_u8, gps_string[pole_u8]);
//			HAL_UART_Transmit(Debug_ch.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
			pole_u8++;
			previous_pole_position_u8 = position+1;
			//_time->hour_int = atoi(time_string) + TIMEZONE;
		}
	}
	_coord->latitude_DMM_dbl	= atof (gps_string[2]);
	_coord->longitude_DMM_dbl	= atof (gps_string[4]);
	_coord->altitude_dbl		= atof (gps_string[9]);
	_coord->satelite_qnt_int	= atoi (gps_string[7]);
}
//***********************************************************

void Convert_from_DMM_to_DD(Coordinates_struct * _coord ) {

	uint32_t	   lat_u32	= (uint32_t)(_coord->latitude_DMM_dbl * 100000.0)		;	//	*10^5
	uint32_t	lat_D0_u32  = lat_u32 / 10000000UL									;	//	/10^7
	uint32_t	lat_D1_u32	= lat_u32 -	(lat_D0_u32 * 10000000UL)					;	//	*10^7
	_coord->latitude_DD_dbl	= (double)lat_D0_u32 + (double)lat_D1_u32/6000000.0		;	//	/60 * 10^5

//	char DebugString[DEBUG_STRING_SIZE];
//	sprintf(DebugString,"%d %d %d %f \r\n", (int)lat_u32, (int)lat_D0_u32, (int)lat_D1_u32, _coord->latitude_DD_dbl );
//	HAL_UART_Transmit(Debug_ch.uart, (uint8_t *)DebugString, strlen(DebugString), 100);

	uint32_t	   long_u32	= (uint32_t)(_coord->longitude_DMM_dbl * 100000.0)		;	//	*10^5
	uint32_t	long_D0_u32 = long_u32 / 10000000UL									;	//	/10^7
	uint32_t	long_D1_u32	= long_u32 -	(long_D0_u32 * 10000000UL)				;	//	*10^7
	_coord->longitude_DD_dbl= (double)long_D0_u32 + (double)long_D1_u32/6000000.0	;	//	/60 * 10^5

}
//***********************************************************
void Calc_rocket_direction (Coordinates_struct * _coord_base, Coordinates_struct * _coord_rocket, Rocket_struct * _rocket) {

	uint32_t base_X_u32     = (uint32_t)(_coord_base  ->longitude_DD_dbl * 1000000.0);
	uint32_t rocket_X_u32   = (uint32_t)(_coord_rocket->longitude_DD_dbl * 1000000.0);

	uint32_t base_Y_u32     = (uint32_t)(_coord_base  ->latitude_DD_dbl  * 1000000.0);
	uint32_t rocket_Y_u32   = (uint32_t)(_coord_rocket->latitude_DD_dbl  * 1000000.0);

	uint32_t   base_alt_u32 = (uint32_t)(_coord_base  ->altitude_dbl * 10.0);
	uint32_t rocket_alt_u32 = (uint32_t)(_coord_rocket->altitude_dbl * 10.0);


	int delta_X_int  = rocket_X_u32 - base_X_u32   ;
	int delta_Y_int  = rocket_Y_u32 - base_Y_u32   ;

	uint32_t koef_Y_u32 = 400UL;
	uint32_t koef_X_u32 = 257UL;	//	400*cos(50);

	if ((delta_X_int > 0) && (delta_Y_int == 0)) {
		_rocket -> quadrant_u8 = 0;
		_rocket -> X_distance_u32 = ((rocket_X_u32 - base_X_u32) * koef_X_u32) / 3600;
		_rocket -> Y_distance_u32 = 0 ;
		_rocket -> azimuth_u32 = 0;
	}

	if ((delta_X_int >= 0) && (delta_Y_int >  0)) {
		_rocket -> quadrant_u8 = 0;
		_rocket -> X_distance_u32 = ((rocket_X_u32 - base_X_u32) * koef_X_u32) / 3600;
		_rocket -> Y_distance_u32 = ((rocket_Y_u32 - base_Y_u32) * koef_Y_u32) / 3600;
		_rocket -> azimuth_u32    = (uint32_t) ((1800.0 * atan2(_rocket -> X_distance_u32, _rocket -> Y_distance_u32)) / 31.415);
	}

	if ((delta_X_int >  0) && (delta_Y_int <= 0)) {
		_rocket->quadrant_u8 = 1;
		_rocket -> X_distance_u32 = ((rocket_X_u32 - base_X_u32) * koef_X_u32) / 3600;
		_rocket -> Y_distance_u32 = ((base_Y_u32 - rocket_Y_u32) * koef_Y_u32) / 3600;
		_rocket -> azimuth_u32    = (uint32_t) ((1800.0 * atan2(_rocket -> Y_distance_u32, _rocket -> X_distance_u32)) / 31.415);
	}

	if ((delta_X_int <= 0) && (delta_Y_int <  0)) {
		_rocket->quadrant_u8 = 2;
		_rocket -> X_distance_u32 = ((base_X_u32 - rocket_X_u32) * koef_X_u32) / 3600;
		_rocket -> Y_distance_u32 = ((base_Y_u32 - rocket_Y_u32) * koef_Y_u32) / 3600;
		_rocket -> azimuth_u32    = (uint32_t) ((1800.0 * atan2(_rocket -> X_distance_u32, _rocket -> Y_distance_u32)) / 31.415);
	}
	if ((delta_X_int <  0) && (delta_Y_int >= 0)) {
		_rocket->quadrant_u8 = 3;
		_rocket -> X_distance_u32 = ((base_X_u32 - rocket_X_u32) * koef_X_u32) / 3600;
		_rocket -> Y_distance_u32 = ((rocket_Y_u32 - base_Y_u32) * koef_Y_u32) / 3600;
		_rocket -> azimuth_u32    = (uint32_t) ((1800.0 * atan2(_rocket -> Y_distance_u32, _rocket -> X_distance_u32)) / 31.415);
	}

	_rocket->abc_distance_u32 = (uint32_t)	sqrt(	(_rocket -> X_distance_u32 * _rocket -> X_distance_u32)
												+	(_rocket -> Y_distance_u32 * _rocket -> Y_distance_u32)	) ;


	int altitude_int = rocket_alt_u32 - base_alt_u32 ;
	if (altitude_int >= 0) {
		_rocket->altitude_U32 = altitude_int / 10;
		_rocket->altitude_err = 0;
	} else {
		_rocket->altitude_U32 = 0;
		_rocket->altitude_err = 1;
	}

	char DebugString[DEBUG_STRING_SIZE];
	sprintf(DebugString,"X:%d Y:%d abc:%d quadr:%d Alt:%d alt_err:%d \tazimuth:%02d\r\n",
			(int) _rocket -> X_distance_u32,
			(int) _rocket -> Y_distance_u32,
			(int) _rocket -> abc_distance_u32,
			(int) _rocket -> quadrant_u8,
			(int) _rocket -> altitude_U32,
			(int) _rocket -> altitude_err,
			(int) _rocket -> azimuth_u32);
	HAL_UART_Transmit(Debug_ch.uart, (uint8_t *)DebugString, strlen(DebugString), 100);

}
//***********************************************************
