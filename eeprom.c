/*
 * eeprom.c
 *
 *  Created on: Feb 3, 2015
 *      Author: wella
 */
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "chip.h"

#define PAGE_ADDR  0x01/* Page number */
/* Read/write buffer (32-bit aligned) */
uint32_t buffer[EEPROM_PAGE_SIZE / sizeof(uint32_t)];

unsigned int i = 0;
volatile unsigned int vterina = 0;
volatile bool start_counting = false;


void vypis(const char * text) {
	int numBytes = 0;
	const char * text_iterace = text;

	while (*text_iterace++) {
		//numBytes = numBytes + 1;
		numBytes++;
	}

	Chip_UART_SendBlocking(LPC_UART0, text, numBytes);

}


static void showTime(RTC_TIME_T *pTime)
{
	 char buffer [100];
	 snprintf ( buffer, 100, "%d:%d:%d\r\n",pTime->time[RTC_TIMETYPE_HOUR],  pTime->time[RTC_TIMETYPE_MINUTE], pTime->time[RTC_TIMETYPE_SECOND] );
	 vypis(buffer);
}

void cteni_casu(void){
	RTC_TIME_T FullTime;
	Chip_RTC_GetFullTime(LPC_RTC, &FullTime);
	showTime(&FullTime);
}
#if 0
void __lpc1788_isr_systick(void)
{
	if (true == start_counting) {
		i++;
		if (i == 10) {
			vterina++;
			i = 0;
		}
	}
}
#endif
void eeprom(void) {
	//***********************************************
	// Inicializace
	//***********************************************

	// Inicializace hodin pro blok iocon, gpio
	Chip_IOCON_Init(LPC_IOCON);
	Chip_GPIO_Init(LPC_GPIO);

	Chip_RTC_Init(LPC_RTC);

	/* Enable RTC (starts increase the tick counter and second counter register) */
	Chip_RTC_Enable(LPC_RTC, ENABLE);

	/*
	 * Initialize UART0 pin connect
	 * P0.2: TXD
	 * P0.3: RXD
	 */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 2, (IOCON_FUNC1 | IOCON_MODE_INACT));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 3, (IOCON_FUNC1 | IOCON_MODE_INACT));

	/* Setup UART for 115.2K8N1 */
	Chip_UART_Init(LPC_UART0);
	Chip_UART_SetBaud(LPC_UART0, 115200);
	Chip_UART_ConfigData(LPC_UART0, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(LPC_UART0, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(LPC_UART0);

	/* Reset and enable FIFOs, FIFO trigger level 3 (14 chars) */
	Chip_UART_SetupFIFOS(LPC_UART0, (UART_FCR_FIFO_EN | UART_FCR_RX_RS |
	UART_FCR_TX_RS | UART_FCR_TRG_LEV3));

	SysTick_Config(120000000 / 10);

	Status status;
	uint8_t *ptr = (uint8_t *) buffer;
	/* Init EEPROM */
	Chip_EEPROM_Init(LPC_EEPROM);
	int x = 0;

	Chip_EEPROM_Read(LPC_EEPROM, 0, PAGE_ADDR, ptr, EEPROM_RWSIZE_8BITS, 10);
	x = ptr[3];
	if(x == 49){
		vypis("\r\n");

		RTC_TIME_T t;
		Chip_RTC_GetFullTime(LPC_RTC, &t);

		if (t.time[RTC_TIMETYPE_HOUR] == ptr[0]) {
			if (t.time[RTC_TIMETYPE_MINUTE] == ptr[1]) {
				if (t.time[RTC_TIMETYPE_SECOND] > ptr[2]) {
					vypis("Passed\r\n");
				} else {
					vypis("Failed\r\n");
				}
			}
			if (t.time[RTC_TIMETYPE_MINUTE] > ptr[1]) {
				vypis("Passed\r\n");
			} else {
				vypis("Failed\r\n");
			}
		}

		if (t.time[RTC_TIMETYPE_HOUR] > ptr[0]) {
			vypis("Passed\r\n");
		}

	} else {
		RTC_TIME_T t;
		Chip_RTC_GetFullTime(LPC_RTC, &t);
		ptr[0] = t.time[RTC_TIMETYPE_HOUR];
		ptr[1] = t.time[RTC_TIMETYPE_MINUTE];
		ptr[2] = t.time[RTC_TIMETYPE_SECOND];
		ptr[3] = '1';

		status = Chip_EEPROM_Write(LPC_EEPROM, 0, PAGE_ADDR, ptr,
				EEPROM_RWSIZE_8BITS, 4);
	}

	/*
	 t.time[RTC_TIMETYPE_HOUR] = hod;
	 t.time[RTC_TIMETYPE_MINUTE] = min;
	 t.time[RTC_TIMETYPE_SECOND] = sec;
	 */
	RTC_TIME_T t;
	Chip_RTC_GetFullTime(LPC_RTC, &t);

	int rtc_start = t.time[RTC_TIMETYPE_SECOND];
	int rtc_end = 0;
	//bool podminka = true;

	vterina = 0;
	start_counting = true;

	while(vterina < 5)
	{};

	start_counting = false;
	Chip_RTC_GetFullTime(LPC_RTC, &t);
	rtc_end = t.time[RTC_TIMETYPE_SECOND];
	int diff;
	if(rtc_end < rtc_start)
	{
	  diff = (60 - rtc_start) + rtc_end;
	}
	else
	{
		diff = rtc_end - rtc_start;
	}



	if((diff >= 4) && (diff<= 6))
	{
		vypis("2.Passed\r\n");
	}


	while (1) {
#if 0
		if (podminka) {

			Chip_RTC_GetFullTime(LPC_RTC, &t);
			rtc_end = t.time[RTC_TIMETYPE_SECOND];
			if (vterina == 5 && rtc_end == (rtc_start + 5)) {
				vypis("2.Passed\r\n");
				podminka = false;
			}

		}
#endif
	}

}


#if 0
	Status status;
	uint8_t *ptr = (uint8_t *) buffer;
	/* Init EEPROM */
	Chip_EEPROM_Init(LPC_EEPROM);

	ptr[0] = 'a';
	ptr[1] = 'b';
	ptr[2] = '\0';

	status = Chip_EEPROM_Write(LPC_EEPROM, 0, PAGE_ADDR, ptr,
			EEPROM_RWSIZE_8BITS, 3);

	ptr[0] = '\0';
	ptr[1] = '\0';
	ptr[2] = '\0';

	/* Read all data from EEPROM page */
	Chip_EEPROM_Read(LPC_EEPROM, 0, PAGE_ADDR, ptr, EEPROM_RWSIZE_8BITS, 10);
#endif


