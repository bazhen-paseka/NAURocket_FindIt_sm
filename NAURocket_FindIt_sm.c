
#include "main.h"
#include "NAURocket_FindIt_sm.h"

//***********************************************************

	extern DMA_HandleTypeDef		hdma_usart2_rx;
	extern DMA_HandleTypeDef		hdma_usart3_rx;

	HAL_StatusTypeDef 				status_res;

	uint8_t rx_circular_buffer[GPS_CH_QNT][RX_BUFFER_SIZE];
	char DebugString[DEBUG_STRING_SIZE];

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

	void Print_No_signal(void);

	void TIM5_Unbreakable_package_Start(void);
	void TIM5_Unbreakable_package_Stop(void);
	void TIM5_time_overflow_reset(void);

	void RTU_3_start(void);
	void RTU_3_stop(void);

	void RTU_2_start(void);
	void RTU_2_stop(void);

	void TIM4_no_signal_Start(void);
	void TIM4_no_signal_Stop(void);
	void TIM4_no_signal_Reset(void);

	void Beep(void);
	void ShutDown(void);

//***********************************************************
//***********************************************************

void NAUR_Init (void){
	LCD_Init();
	LCD_SetRotation(1);
	LCD_SetCursor(0, 0);
	LCD_FillScreen(ILI92_BLACK);
	LCD_SetTextColor(ILI92_GREEN, ILI92_BLACK);

	DebugH.uart = &huart5;

	sprintf(DebugString,"\r\n\r\n");
	HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);

	int soft_version_arr_int[3];
	soft_version_arr_int[0] = ((SOFT_VERSION) / 100) %10 ;
	soft_version_arr_int[1] = ((SOFT_VERSION) /  10) %10 ;
	soft_version_arr_int[2] = ((SOFT_VERSION)      ) %10 ;
	sprintf(DebugString,"NAUR Find It F446\r\n2020-April-21 v%d.%d.%d \r\nfor_debug UART5 115200/8-N-1\r\n",
			soft_version_arr_int[0], soft_version_arr_int[1], soft_version_arr_int[2]);
	HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
	LCD_Printf("%s",DebugString);

	RingBuffer_DMA_Init(&rx_buffer[3], &hdma_usart3_rx, rx_circular_buffer[3], RX_BUFFER_SIZE);  	// Start UART receive
	status_res = HAL_UART_Receive_DMA(&huart3, rx_circular_buffer[3], RX_BUFFER_SIZE);  	// how many bytes in buffer
	sprintf(DebugString, "ch3: UART_3_DMA status:%d\r\n", (int)status_res);
	HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);

	RingBuffer_DMA_Init(&rx_buffer[2], &hdma_usart2_rx, rx_circular_buffer[2], RX_BUFFER_SIZE);  	// Start UART receive
	status_res = HAL_UART_Receive_DMA(&huart2, rx_circular_buffer[2], RX_BUFFER_SIZE);  	// how many bytes in buffer
	sprintf(DebugString, "ch2: UART_2_DMA status:%d\r\n", (int)status_res);
	HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);

	FATFS_SPI_Init(&hspi1);	/* Initialize SD Card low level SPI driver */
	static uint8_t try_u8;
	do{
		fres = f_mount(&USERFatFS, "", 1);	/* try to mount SDCARD */
		if (fres == FR_OK) {
			sprintf(DebugString,"\r\nSDcard mount - Ok \r\n");
			HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
			LCD_Printf("%s",DebugString);
		} else {
			f_mount(NULL, "", 0);			/* Unmount SDCARD */
			Error_Handler();
			try_u8++;
			sprintf(DebugString,"%d)SDcard mount: Failed  Error: %d\r\n", try_u8, fres);
			HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
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
	HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);

	if (hdma_usart2_rx.State == HAL_DMA_STATE_ERROR) {
			HAL_UART_DMAStop(&huart2);
			status_res = HAL_UART_Receive_DMA(&huart2, rx_circular_buffer[2], RX_BUFFER_SIZE);
			sprintf(DebugString, "ch2: UART_2_DMA status:%d\r\n", (int)status_res);
	} else {
		sprintf(DebugString, "ch2: UART_2_DMA status - Ok.\r\n");
	}
	HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);

	//***********************************************************

	GPS[2].channel = GPS_CH_2;
	GPS[3].channel = GPS_CH_3;

	LCD_FillScreen(ILI92_BLACK);
	TIM5_Unbreakable_package_Start();
	TIM4_no_signal_Start();

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

			RTU_2_reset();
			RTU_2_start();

			RTU_3_reset();
			RTU_3_start();

			TIM5_time_overflow_reset();
			sm_stage =SM_READ_FROM_RINGBUFFER;
		} break;

		//***********************************************************

		case SM_READ_FROM_RINGBUFFER: {
			Read_from_RingBuffer(&GPS[3], &rx_buffer[3] );
			Read_from_RingBuffer(&GPS[2], &rx_buffer[2] );
			sm_stage = SM_CHECK_FLAGS;
		} break;

		//***********************************************************

		case SM_CHECK_FLAGS: {
			if (GPS[2].end_of_UART_packet == 1) {
				GPS[2].end_of_UART_packet = 0;
				if ((GPS[2].length_int > 0) && (GPS[2].UART_packet_ready_u8 == 0)) {
					RTU_2_stop();
					TIM4_no_signal_Reset();
					GPS[2].UART_packet_ready_u8 = 1;
				}
			}

			if (GPS[3].end_of_UART_packet == 1) {
				GPS[3].end_of_UART_packet = 0;
				if ((GPS[3].length_int > 0) && (GPS[3].UART_packet_ready_u8 == 0)) {
					RTU_3_stop();
					TIM4_no_signal_Reset();
					GPS[3].UART_packet_ready_u8 = 1;
				}
			}

			if ((GPS[2].UART_packet_ready_u8 == 1) && (GPS[3].UART_packet_ready_u8 == 1)) {
				sm_stage = SM_PREPARE_FILENAME;
				break;
			}

			if ((GPS[3].packet_overflow == 1) || (GPS[3].time_overflow_u8 == 1)) {
				RTU_3_stop();
				TIM4_no_signal_Reset();
				Beep();
				sm_stage = SM_PREPARE_FILENAME;
				break;
			}

			if (SD.shudown_button_pressed == 1) {
				ShutDown();
				sm_stage = SM_FINISH;	//	but it must Reset by IWDT
				break;
			}

			if (GPS[3].no_signal == 1) {
				Print_No_signal();
				sm_stage = SM_FINISH;
				break;
			}

			sm_stage = SM_READ_FROM_RINGBUFFER;
		} break;
	//***********************************************************

		case SM_PREPARE_FILENAME: {
			Prepare_filename(&SD);
			sm_stage = SM_WRITE_SDCARD;
		} break;
	//***********************************************************

		case SM_WRITE_SDCARD: {
			SDcard_Write(&GPS[3], &SD);
			sm_stage = SM_PRINT_ALL_INFO;
		} break;
	//***********************************************************

		case SM_PRINT_ALL_INFO: {
			Print_GPS_to_UART(&GPS[2]) ;
			Print_GPS_to_UART(&GPS[3]) ;
			Print_SD_Card_to_UART(&SD) ;

			LCD_FillScreen(ILI92_BLACK);
			LCD_SetCursor(0, 0);
			Print_GPS_to_LCD(&GPS[2]);
			Print_GPS_to_LCD(&GPS[3]);

			sm_stage = SM_FINISH;
		} break;
	//***********************************************************

		case SM_FINISH: {
			sm_stage = SM_START;
		} break;
	//***********************************************************

		default: {
			sm_stage = SM_START;
		} break;
	}	//			switch (sm_stage)
}
//***********************************************************

void Print_GPS_to_LCD(GPS_struct * _gps) {
	//	LCD_SetCursor(0, 95*(_time->seconds_int%2));
	//	LCD_FillScreen(ILI92_BLACK);
	//	LCD_SetCursor(0, 0);
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
	snprintf(DebugString, _gps->length_int +4,"%d) %s", (int)_gps->channel, _gps->string);
	HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);

	if (_gps->time_overflow_u8 == 1) {
		sprintf(DebugString,">> ## ch%d Time_overflow ##\r\n", (int)_gps->channel);
		HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
	}

	if (_gps->packet_overflow == 1) {
		sprintf(DebugString,">> ## ch%d Packet_overflow ##\r\n", (int)_gps->channel);
		HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
	}
}
//***********************************************************

void Print_SD_Card_to_UART(SD_Card_struct * _sd ) {
	sprintf(DebugString,">> file_name:%s; size: %d; SD_write: %d\r\n\r\n",
												_sd->file_name_char,
												(int)_sd->file_size,
												_sd->write_status		);
	HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
}
//***********************************************************

void TIM5_Unbreakable_package_Start(void) {
	HAL_TIM_Base_Start_IT(&htim5);
	HAL_TIM_Base_Start   (&htim5);
}
//***********************************************************

void TIM5_Unbreakable_package_Stop(void) {
	HAL_TIM_Base_Stop_IT(&htim5);
	HAL_TIM_Base_Stop   (&htim5);
}
//***********************************************************

void TIM5_time_overflow_reset(void) {
	TIM5->CNT = 0;
}
//***********************************************************

void RTU_3_start(void) {
	HAL_TIM_Base_Start_IT(&htim3);
	HAL_TIM_Base_Start   (&htim3);
}
//***********************************************************

void RTU_3_stop(void) {
	HAL_TIM_Base_Stop_IT(&htim3);
	HAL_TIM_Base_Stop   (&htim3);
}
//***********************************************************

void RTU_3_reset(void) {
	TIM3->CNT = 0;
}
//***********************************************************

void RTU_2_start(void){
	HAL_TIM_Base_Start_IT(&htim2);
	HAL_TIM_Base_Start   (&htim2);
}
//***********************************************************

void RTU_2_stop(void) {
	HAL_TIM_Base_Stop_IT(&htim2);
	HAL_TIM_Base_Stop   (&htim2);
}
//***********************************************************

void RTU_2_reset(void) {
	TIM2->CNT = 0;
}

//***********************************************************
void TIM4_no_signal_Start(void) {
	HAL_TIM_Base_Start_IT(&htim4);
	HAL_TIM_Base_Start   (&htim4);
}
//***********************************************************

void TIM4_no_signal_Stop(void) {
	HAL_TIM_Base_Stop_IT(&htim4);
	HAL_TIM_Base_Stop   (&htim4);
}
//***********************************************************

void TIM4_no_signal_Reset(void) {
	TIM4->CNT = 0;
}
//***********************************************************

void ShutDown(void) {
	LCD_FillScreen(ILI92_BLACK);
	LCD_SetCursor(0, 0);
	for (int i=5; i>=0; i--) {
		sprintf(DebugString,"Remove SD-card! Left %d sec.\r\n", i);
		HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
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
	snprintf(DebugString, _gps->length_int+1,"%s", _gps->string);
#if (NAUR_FI_F446 == 1)
	fres = f_open(&USERFile, _sd->file_name_char, FA_OPEN_APPEND | FA_WRITE );			/* Try to open file */
#elif (NAUR_FI_F103 == 1)
	fres = f_open(&USERFile, _sd->file_name_char, FA_OPEN_ALWAYS | FA_WRITE);
	fres += f_lseek(&USERFile, f_size(&USERFile));
#endif
	_sd->write_status = fres;
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
	_gps->length_int			= 0 ;
	_gps->end_of_UART_packet	= 0 ;
	_gps->no_signal	 			= 0 ;
	_gps->time_overflow_u8		= 0 ;
	_gps->packet_overflow		= 0 ;
	_gps->UART_packet_ready_u8	= 0 ;
}
//***********************************************************

void Clear_SD_Card_struct(SD_Card_struct * _sd) {
	_sd->write_status			= 0 ;
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
			_gps->packet_overflow = 1;
			return;
		}
	}
}
//***********************************************************

void Set_flag_Shudown_button_pressed(void) {
	SD.shudown_button_pressed	= 1 ;
}
//***********************************************************

void Set_flag_End_of_UART_2_packet(void) {
	GPS[2].end_of_UART_packet = 1;
}
//***********************************************************

void Set_flag_End_of_UART_3_packet(void) {
	GPS[3].end_of_UART_packet = 1;
}
//***********************************************************

void Set_flag_No_signal(void) {
	GPS[3].no_signal = 1;
}
//***********************************************************

void Set_flag_time_overflow_package(void) {
	GPS[3].time_overflow_u8 = 1;
}
//***********************************************************

void Print_No_signal(void) {
	sprintf(DebugString,">> ## NO signal from GPS ##\r\n");
	HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
	LCD_FillScreen(ILI92_BLACK);
	LCD_SetCursor(0, 0);
	LCD_Printf("%s", DebugString);
	Beep();
}
//***********************************************************
