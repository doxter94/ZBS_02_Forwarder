#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "chip.h"
#include "ethernet.h"
#include "uart_receive.h"

/*****************************************************************************
 * Private functions
 ****************************************************************************/

 static void delay(uint32_t i);

 /* Delay in miliseconds  (cclk = 120MHz) */
 static void delayMs(uint32_t ms) {
 	delay(ms * 30030);
 }

int main(void) {
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
	//uart_debug("Start!\r\n",LPC_UART0);
	while (1) {
		ethernet_transmit();
	}

	return EXIT_FAILURE;
}


