#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "chip.h"
#include "ethernet.h"
#include "uart_receive.h"

#define MAX_LENGHT 127
#define LAST_CHAR 0x0A

/*****************************************************************************
 * Private functions
 ****************************************************************************/

 static void delay(uint32_t i);


 /* Delay in miliseconds  (cclk = 120MHz) */
 static void delayMs(uint32_t ms) {
 	delay(ms * 30030);
 }

static void uart_write(const char * text, LPC_USART_T *pUART){
	uint16_t numBytes = 0;
	const char * text_iterace = text;

	while (*text_iterace++) {
		numBytes++;
	}

	Chip_UART_SendBlocking(pUART, text, numBytes);
}


int main(void) {
	delayMs(150);
	//***********************************************
	// Inicializace
	//***********************************************
	Chip_IOCON_Init(LPC_IOCON);
	Chip_GPIO_Init(LPC_GPIO);

	enet_init();
	setup_uarts();

	//***********************************************
	// Hlavni smycka
	//***********************************************
	uart_write("Start!\r\n",LPC_UART0);

	while (1) {

	}

	return EXIT_FAILURE;
}
