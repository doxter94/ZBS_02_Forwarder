#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "chip.h"
#include "ethernet.h"
#include "uart_receive.h"

/* Page used for storage */
#define PAGE_ADDR       0x01/* Page number */


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
	eeprom_init();
	setup_uarts();

	//***********************************************
	// Hlavni smycka
	//***********************************************

	while (1) {

		eeprom_write();
		ethernet_transmit();

	}

	return EXIT_FAILURE;
}


