#if 0
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "chip.h"

#define BUFFER_SIZE 128
#define FIFO_SIZE 16
#define UART0 (LPC_UART0)
#define UART1 (LPC_UART1)



typedef struct {
	uint8_t buffer[BUFFER_SIZE];
	volatile unsigned int producer;
	volatile unsigned int consumer;
} Buffer_t;

static Buffer_t tx_u0_KB;
static Buffer_t tx_u1_KB;

static bool full_buffer(Buffer_t * buffer) {
	return (((buffer->producer + 1) % BUFFER_SIZE) == buffer->consumer);
}

static bool empty_buffer(Buffer_t * buffer) {
	return ((buffer->producer) == buffer->consumer);
}

static void uart_isr_handler(LPC_USART_T *called_from_uart,LPC_USART_T *write_to_uart,Buffer_t *called_from_buf,Buffer_t *write_to_buf){

	uint32_t pom;
	//Nejake preruseni penduje na uartu0
	while (!(UART_IIR_INTSTAT_PEND & (pom = Chip_UART_ReadIntIDReg(called_from_uart)))) {
		//zjistime jake
		pom = (pom & 0xE) >> 1;
		switch (pom)
		{
		//THRE ( nastane pokud je aurt0 tx fifo prazdne)
		case 0x1: {
			// kdyz je prazdny buffer u1
			if (empty_buffer(write_to_buf)) {
				Chip_UART_IntDisable(called_from_uart, UART_IER_THREINT);
				break;
			}
			int i = FIFO_SIZE;
			//opakuj dokud buffer u1 nebude prazdny nebo nepretece FIFO tx auartu0
			while (!empty_buffer(write_to_buf) && i > 0) {
				i--;
				Chip_UART_SendByte(called_from_uart, write_to_buf->buffer[write_to_buf->consumer]);
				write_to_buf->consumer = (write_to_buf->consumer + 1) % BUFFER_SIZE;
			}
			break;
		}
		//RDA nebo CTI (pokud rx fifa uatru0 obsahuje aspon 1 znak..)
		case 0x6:

		case 0x2: {
			//pokud je buffer u0 prazdny a fifo tx uartu1 je prazdne
			if (empty_buffer(called_from_buf) && (UART_LSR_THRE & Chip_UART_ReadLineStatus(write_to_uart))) {
				int fs = FIFO_SIZE;
				// 1 = RX FIFO UART0 is not empty (contains data)
				while ((UART_LSR_RDR & Chip_UART_ReadLineStatus(called_from_uart))) {
					uint8_t x;
					// x = single byte from UART0 RX FIFO
					x = Chip_UART_ReadByte(called_from_uart);
					if (x != '0') {
						if (fs > 0) {
							Chip_UART_SendByte(write_to_uart, x);
							fs--;
						} else {
							if (!full_buffer(called_from_buf)) {
								called_from_buf->buffer[called_from_buf->producer] = x;
								called_from_buf->producer = (called_from_buf->producer + 1)% BUFFER_SIZE;
							}
						}
					}
				}
			}
			else {
				//opakovani dokud nebude buffer u0 plny a dokavad budou v rx fifu uatru0 data
				while ((!full_buffer(called_from_buf)) && (UART_LSR_RDR & Chip_UART_ReadLineStatus(called_from_uart))) {
					uint8_t x;
					x = Chip_UART_ReadByte(called_from_uart);
					if (x != '0') {
						called_from_buf->buffer[called_from_buf->producer] = x;
						called_from_buf->producer = (called_from_buf->producer + 1)% BUFFER_SIZE;
					}
				}
			}
			Chip_UART_IntEnable(write_to_uart, UART_IER_THREINT);

			break;
		}
		}
	}
}
void __lpc1788_isr_uart0(void) {
	uart_isr_handler(UART0,UART1,&tx_u0_KB,&tx_u1_KB);
}


void __lpc1788_isr_uart1(void) {
	uart_isr_handler(UART1,UART0,&tx_u1_KB,&tx_u0_KB);
}

void forwarder_init(void) {

	// Inicializace hodin pro blok iocon, gpio
	Chip_IOCON_Init(LPC_IOCON);
	Chip_GPIO_Init(LPC_GPIO);
	//Initialize UART0/1 pin connect
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 2, (IOCON_FUNC1 | IOCON_MODE_INACT));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 3, (IOCON_FUNC1 | IOCON_MODE_INACT));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 15, (IOCON_FUNC1 | IOCON_MODE_INACT));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 16, (IOCON_FUNC1 | IOCON_MODE_INACT));

	Chip_UART_Init(UART0);
	Chip_UART_SetBaud(UART0, 115200);
	Chip_UART_ConfigData(UART0, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(UART0, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(UART0);

	Chip_UART_Init(UART1);
	Chip_UART_SetBaud(UART1, 115200);
	Chip_UART_ConfigData(UART1, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(UART1, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(UART1);

	Chip_UART_SetupFIFOS(UART0, (UART_FCR_FIFO_EN | UART_FCR_RX_RS |
	UART_FCR_TX_RS | UART_FCR_TRG_LEV3));
	Chip_UART_SetupFIFOS(UART1, (UART_FCR_FIFO_EN | UART_FCR_RX_RS |
	UART_FCR_TX_RS | UART_FCR_TRG_LEV3));

	Chip_UART_IntEnable(UART0, (UART_IER_RBRINT));
	Chip_UART_IntEnable(UART1, (UART_IER_RBRINT));

	NVIC_EnableIRQ(UART0_IRQn);
	NVIC_EnableIRQ(UART1_IRQn);

}

#endif