
#include "main.h"
#include "NAURocket_FindIt_sm.h"

//***********************************************************

void Print(NEO6_struct * _neo6, GGA_struct * _gga, CheckSum_struct * _cs, Time_struct * _time, SD_Card_struct * _sd)
{
	LCD_SetCursor(0, 95*(_time->seconds_int%2));
	sprintf(DebugString,"%s", _gga->string);
	HAL_UART_Transmit(&huart5, (uint8_t *)DebugString, strlen(DebugString), 100);
	LCD_Printf("%s", DebugString);

	sprintf(DebugString,"%d/%d/%d/cs%d; %s; SD_write: %d\r\n",
								_gga->Neo6_start,
								_gga->Neo6_end,
								_gga->length,
								_cs->status_flag,
								_sd->filename,
								_sd->write_status);
	HAL_UART_Transmit(&huart5, (uint8_t *)DebugString, strlen(DebugString), 100);

	sprintf(DebugString,"%s SD_wr %d\r\n", _sd->filename, _sd->write_status);
	LCD_SetCursor(0, 195);
	LCD_Printf("%s", DebugString);
}

void TIM_Start(void)
{
	TIM3->CNT = 0;
	HAL_TIM_Base_Start_IT(&htim3);
	HAL_TIM_Base_Start(&htim3);
}

//***********************************************************

void TIM_Stop(void)
{
	HAL_TIM_Base_Stop_IT(&htim3);
	HAL_TIM_Base_Stop(&htim3);
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



void Get_time_from_GGA_string(GGA_struct * _gga, Time_struct * _time)
{
	if (_time->updated_flag == 1)
	{
		return;
	}

	_time->updated_flag = 1;

	#define TIME_SIZE	5
	char time_string[TIME_SIZE];

//	for (int i=0; i<TIME_SIZE; i++)
//	{
//		time_string[i] = 0x00;
//	}

	memset(time_string,0,TIME_SIZE);

	memcpy(time_string, &_gga->string[7], 2);
	_time->hour_int = atoi(time_string) + TIMEZONE;

	memcpy(time_string, &_gga->string[9], 2);
	_time->minutes_int = atoi(time_string);

	memcpy(time_string, &_gga->string[11], 2);
	_time->seconds_int = atoi(time_string);
}

//***********************************************************

uint8_t Calc_SheckSum_GGA(GGA_struct * _gga, CheckSum_struct * _cs)
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
		return 1;
	}
	return 0;
}

//***********************************************************

void ShutDown(void)
{
	LCD_FillScreen(0x0000);
	LCD_SetCursor(0, 0);
	for (int i=5; i>=0; i--)
	{
		sprintf(DebugString,"SHUTDOWN %d\r\n", i);
		HAL_UART_Transmit(&huart5, (uint8_t *)DebugString, strlen(DebugString), 100);
		LCD_Printf("%s",DebugString);
		HAL_GPIO_WritePin(BEEPER_GPIO_Port, BEEPER_Pin, GPIO_PIN_RESET);
		HAL_Delay(800);
		HAL_GPIO_WritePin(BEEPER_GPIO_Port, BEEPER_Pin, GPIO_PIN_SET);
		HAL_Delay(300);
		HAL_IWDG_Refresh(&hiwdg);
	}
	LCD_FillScreen(0x0000);
	LCD_SetCursor(0, 0);
}

//***********************************************************


void Write_SD_card(GGA_struct * _gga, SD_Card_struct * _sd)
{
	fres = f_open(&USERFile, _sd->filename, FA_OPEN_APPEND | FA_WRITE );			/* Try to open file */
	_sd->write_status = fres;
	if (fres == FR_OK)
	{
		f_printf(&USERFile, "%s", _gga->string);	/* Write to file */
		f_close(&USERFile);	/* Close file */
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

void Force_copy_GGA(NEO6_struct * _neo6, GGA_struct * _gga)
{
	memcpy(_gga->string, &_neo6->string[103], 75);
}

//***********************************************************

void Correct_copy_GGA(NEO6_struct * _neo6, GGA_struct * _gga)
{
	memcpy(_gga->string, &_neo6->string[_gga->Neo6_start], _gga->length );
}

//***********************************************************
