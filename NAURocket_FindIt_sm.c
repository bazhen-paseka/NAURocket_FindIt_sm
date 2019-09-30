
#include "main.h"
#include "NAURocket_FindIt_sm.h"

//***********************************************************

	uint8_t rx_circular_buffer[RX_BUFFER_SIZE];
	extern DMA_HandleTypeDef hdma_usart3_rx;
	char DebugString[DEBUG_STRING_SIZE];

//***********************************************************
//***********************************************************

	void Clear_variables(NEO6_struct * _neo6, Flags_struct * _flag, SD_Card_struct * _sd);
	void Read_from_RingBuffer(NEO6_struct * _neo6, RingBuffer_DMA * buffer, Flags_struct * _flag);

	void Prepare_filename(SD_Card_struct * _sd);
	void SDcard_Write(NEO6_struct * _neo6, SD_Card_struct * _sd);

	void Print_all_info(NEO6_struct * _neo6, SD_Card_struct * _sd, Flags_struct * _flag);
	void Print_No_signal(void);

	void TIM2_Unbreakable_package_Start(void);
	void TIM2_Unbreakable_package_Stop(void);
	void TIM2_Unbreakable_package_Reset(void);

	void TIM3_end_of_packet_Start(void);
	void TIM3_end_of_packet_Stop(void);

	void TIM4_no_signal_Start(void);
	void TIM4_no_signal_Stop(void);
	void TIM4_no_signal_Reset(void);

	void Beep(void);
	void ShutDown(void);

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
	sprintf(DebugString,"NAUR Find It F446\r\n2019 lite3.0.0\r\nfor_debug UART5 115200/8-N-1\r\n");
#elif (NAUR_FI_F103 == 1)
	sprintf(DebugString,"NAUR Find It F103\r\n2019 lite3.0.0\r\nfor_debug UART5 115200/8-N-1\r\n");
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

	static uint8_t try_u8;
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

	#if (NAUR_FI_F446 == 1)
		if (hdma_usart3_rx.State == HAL_DMA_STATE_ERROR)
		{
			HAL_UART_DMAStop(&huart3);
			HAL_UART_Receive_DMA(&huart3, rx_circular_buffer, RX_BUFFER_SIZE);
		}
	#endif

#if (NAUR_FI_F446 == 1)
	LCD_FillScreen(ILI92_BLACK);
#elif (NAUR_FI_F103 == 1)
	LCD_FillScreen(ILI92_WHITE);
#endif
	TIM2_Unbreakable_package_Start();
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
			Clear_variables(&NEO6, &FLAG, &SD);
			TIM3_end_of_packet_Reset();
			TIM3_end_of_packet_Start();
			TIM2_Unbreakable_package_Reset();
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
				if (NEO6.length_int == 0)
				{
					sm_stage = SM_FINISH;
					break;
				}
				TIM4_no_signal_Reset();
				sm_stage = SM_PREPARE_FILENAME;
				break;
			}

			if ((FLAG.packet_overflow == 1) || (FLAG.unbreakable_package == 1))
			{
				TIM3_end_of_packet_Stop();
				TIM4_no_signal_Reset();
				Beep();
				sm_stage = SM_PREPARE_FILENAME;
				break;
			}

			if (FLAG.shudown_button_pressed == 1)
			{
				ShutDown();
				sm_stage = SM_FINISH;	//	but it must Reset by IWDT
				break;
			}

			if (FLAG.no_signal == 1)
			{
				Print_No_signal();
				sm_stage = SM_FINISH;
				break;
			}

			sm_stage = SM_READ_FROM_RINGBUFFER;
		} break;
	//***********************************************************

		case SM_PREPARE_FILENAME:
		{
			HAL_GPIO_WritePin(TEST_PC6_GPIO_Port, TEST_PC6_Pin, GPIO_PIN_SET);
			Prepare_filename(&SD);
			sm_stage = SM_WRITE_SDCARD;
		} break;
	//***********************************************************

		case SM_WRITE_SDCARD:
		{
			SDcard_Write(&NEO6, &SD);
			sm_stage = SM_PRINT_ALL_INFO;
		} break;
	//***********************************************************

		case SM_PRINT_ALL_INFO:
		{
			Print_all_info(&NEO6,&SD, &FLAG);
			sm_stage = SM_FINISH;
		} break;
	//***********************************************************

		case SM_FINISH:
		{
			HAL_GPIO_WritePin(TEST_PC6_GPIO_Port, TEST_PC6_Pin, GPIO_PIN_RESET);
			sm_stage = SM_START;
		} break;
	//***********************************************************

		default:
		{
			sm_stage = SM_START;
		} break;
	}	//			switch (sm_stage)
}
//***********************************************************

void Print_all_info(NEO6_struct * _neo6, SD_Card_struct * _sd, Flags_struct * _flag)
{
	snprintf(DebugString, _neo6->length_int+3,"\r\n%s", _neo6->string);
	HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);

	sprintf(DebugString,"\r\n>> File name:%s; size: %d; SD_write: %d\r\n",
												_sd->file_name_char,
												(int)_sd->file_size,
												_sd->write_status		);
	HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);

	if (FLAG.unbreakable_package == 1)
	{
		sprintf(DebugString,">> ## Unbreakable package ##\r\n");
		HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
	}

	if (FLAG.packet_overflow == 1)
	{
		sprintf(DebugString,">> ## Packet overflow ##\r\n");
		HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
	}


//	LCD_SetCursor(0, 95*(_time->seconds_int%2));
	LCD_FillScreen(ILI92_BLACK);
	LCD_SetCursor(0, 0);
	uint8_t lcd_circle = _neo6->length_int / 254;
	char tmp_str[0xFF];
	for (int i = 0; i < lcd_circle; i++)
	{
		memcpy(tmp_str, &_neo6->string[i*254], 255-1);
		snprintf(DebugString, 255,"%s", tmp_str);
		LCD_Printf("%s", DebugString);
	}
	uint8_t tmp_ctr_size = _neo6->length_int - 254 * lcd_circle;

	memcpy(tmp_str, &_neo6->string[lcd_circle*254], tmp_ctr_size);
	snprintf(DebugString, tmp_ctr_size + 1, "%s", tmp_str);
	LCD_Printf("%s", DebugString);
}
//***********************************************************

void TIM2_Unbreakable_package_Start(void)
{
	HAL_TIM_Base_Start_IT(&htim2);
	HAL_TIM_Base_Start(&htim2);
}
//***********************************************************

void TIM2_Unbreakable_package_Stop(void)
{
	HAL_TIM_Base_Stop_IT(&htim2);
	HAL_TIM_Base_Stop(&htim2);
}
//***********************************************************

void TIM2_Unbreakable_package_Reset(void)
{
	TIM2->CNT = 0;
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

void ShutDown(void)
{
#if (NAUR_FI_F446 == 1)
	LCD_FillScreen(ILI92_BLACK);
#elif (NAUR_FI_F103 == 1)
	LCD_FillScreen(ILI92_WHITE);
#endif

	LCD_SetCursor(0, 0);
	for (int i=5; i>=0; i--)
	{
		sprintf(DebugString,"Remove SD-card! Left %d sec.\r\n", i);
		HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
		LCD_Printf("%s",DebugString);
		HAL_GPIO_WritePin(BEEPER_GPIO_Port, BEEPER_Pin, GPIO_PIN_SET);
		HAL_Delay(1000);
		HAL_GPIO_WritePin(BEEPER_GPIO_Port, BEEPER_Pin, GPIO_PIN_RESET);
		HAL_Delay(400);
	}
#if (NAUR_FI_F446 == 1)
	LCD_FillScreen(ILI92_BLACK);
#elif (NAUR_FI_F103 == 1)
	LCD_FillScreen(ILI92_WHITE);
#endif

	LCD_SetCursor(0, 0);
}
//***********************************************************

void SDcard_Write(NEO6_struct * _neo6, SD_Card_struct * _sd)
{
	snprintf(DebugString, _neo6->length_int+1,"%s", _neo6->string);
#if (NAUR_FI_F446 == 1)
	fres = f_open(&USERFile, _sd->file_name_char, FA_OPEN_APPEND | FA_WRITE );			/* Try to open file */
#elif (NAUR_FI_F103 == 1)
	fres = f_open(&USERFile, _sd->file_name_char, FA_OPEN_ALWAYS | FA_WRITE);
	fres += f_lseek(&USERFile, f_size(&USERFile));
#endif
	_sd->write_status = fres;
	if (fres == FR_OK)
	{
		HAL_IWDG_Refresh(&hiwdg);
		f_printf(&USERFile, "%s", DebugString);	/* Write to file */
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

void Clear_variables(NEO6_struct * _neo6, Flags_struct * _flag, SD_Card_struct * _sd)
{
	_sd->write_status			= 0 ;
	_neo6->length_int			= 0 ;

	_flag->end_of_UART_packet	= 0 ;
	_flag->no_signal	 		= 0 ;
	_flag->unbreakable_package	= 0 ;
	_flag->packet_overflow		= 0 ;
}
//***********************************************************

void Prepare_filename(SD_Card_struct * _sd)
{
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

void Read_from_RingBuffer(NEO6_struct * _neo6, RingBuffer_DMA * _rx_buffer, Flags_struct * _flag)
{
  	uint32_t rx_count = RingBuffer_DMA_Count(_rx_buffer);
	while (rx_count--)
	{
		_neo6->string[_neo6->length_int] = RingBuffer_DMA_GetByte(_rx_buffer);
		_neo6->length_int++;
		if (_neo6->length_int > MAX_CHAR_IN_NEO6)
		{
			_flag->packet_overflow = 1;
			return;
		}
	}
}
//***********************************************************

void Update_flag_Shudown_button_pressed(void)
{
	FLAG.shudown_button_pressed	= 1 ;
}
//***********************************************************

void Update_flag_End_of_UART_packet(void)
{
	FLAG.end_of_UART_packet = 1;
}
//***********************************************************

void Update_flag_No_signal(void)
{
	FLAG.no_signal = 1;
}
//***********************************************************

void Update_flag_Unbreakable_package(void)
{
	FLAG.unbreakable_package = 1;
}
//***********************************************************

void Print_No_signal(void)
{
	sprintf(DebugString,">> ## NO signal from GPS ##\r\n");
	HAL_UART_Transmit(DebugH.uart, (uint8_t *)DebugString, strlen(DebugString), 100);
	LCD_FillScreen(ILI92_BLACK);
	LCD_SetCursor(0, 0);
	LCD_Printf("%s", DebugString);
	Beep();
}
//***********************************************************
//***********************************************************
//***********************************************************
