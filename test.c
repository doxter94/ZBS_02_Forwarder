/*
 * test.c
 *
 *  Created on: Jul 16, 2015
 *      Author: jirka
 */


	//***********************************************
	// Includes and private functions
	//***********************************************

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include "chip.h"

#define UART0 (LPC_UART0)
#define UART1 (LPC_UART1)
#define UART2 (LPC_UART2)

#define BUF_SIZE 128
#define LAST_CHAR 0x0A

static uint8_t b;


typedef struct {
	uint8_t buffer[BUF_SIZE];
	volatile unsigned int producer;
	volatile unsigned int consumer;
} Buffer_t;

static Buffer_t buf_u0;

static bool full_buffer(Buffer_t * buffer) {
	return (((buffer->producer + 1) % BUF_SIZE) == buffer->consumer);
}

static bool empty_buffer(Buffer_t * buffer) {
	return ((buffer->producer) == buffer->consumer);
}

static void uart_write(const char * text, LPC_USART_T *pUART){
	uint16_t numBytes = 0;
	const char * text_iterace = text;

	while (*text_iterace++) {
		numBytes++;
	}

	Chip_UART_SendBlocking(pUART, text, numBytes);
}


/* start - zacatek cteni (polozka v bufferu zacina od nuly)
 * end - konec cteni (BUF_SIZE a vys brano jako 128) */
void read_buffer(uint8_t start, uint8_t end){ /* Zatim pro test vypis na uart2 */
	if(start >= BUF_SIZE)
			return;

	if(end >= BUF_SIZE)
		end = 128;

	uart_write("Buffer: ",UART2);
	int i;
	for(i = start;i <= end;i++){
		if(buf_u0.buffer[i] == LAST_CHAR){
			uart_write("\r\n",UART2);
			break;
		}

		Chip_UART_SendByte(UART2,buf_u0.buffer[i]);
	}
}


	//***********************************************
	// void __lpc1788_isr_uart0(void)
	// Test prichozich dat na uart0! s inicializaci se pocita
	//***********************************************

enum transmit_state{
	SEND_DATA,
	WHOLE_MESSAGE,
	ENET_COMPLETE
};

static enum transmit_state uart0_data = SEND_DATA;

	void priklad1(void){ /* Nazev priklad1 bude nahrazen __lpc1788_isr_uart0 */
		/* Dokud budou na uartu data */
		while (UART_LSR_RDR & Chip_UART_ReadLineStatus(UART0)) {
			switch (uart0_data) {
			/* Jedine pokud je buffer prazdny nazacatku */
			case SEND_DATA: {
				//Kontrola bufferu
				/*if (full_buffer(&buf_u0)) {
					uart0_data = WHOLE_MESSAGE;
					break;
				}*/

				b = Chip_UART_ReadByte(UART0);
				if(b == LAST_CHAR){
					uart0_data = WHOLE_MESSAGE;
					break;
				}
				buf_u0.buffer[buf_u0.producer] = b;
				buf_u0.producer = (buf_u0.producer +1) % BUF_SIZE;



				break;
			}
			case WHOLE_MESSAGE: {
				/* Odeslani na ethernet (inicializace enetu bude
				 *  nazacatku funkce main spolu s initem uartu */



				break;
			}
			case ENET_COMPLETE: {
				break;
			}

			}
		}
	}

	//***********************************************
	//
	//***********************************************



	//***********************************************
	//
	//***********************************************



	//***********************************************
	//
	//***********************************************



	//***********************************************
	//
	//***********************************************



	//***********************************************
	//
	//***********************************************


