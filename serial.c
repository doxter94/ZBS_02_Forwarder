#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "chip.h"

#define BUF_SIZE 64
#define FIFO_SIZE 16
#define UART0 (LPC_UART0)
#define UART1 (LPC_UART1)
#define UART2 (LPC_UART2)

static LPC_USART_T *write_uart[3];

enum data{
	SYNC_BYTE,
	DEST_UART,
	LENGHT_OF_DATA,
	RECEIVE_DATA
};

enum data uart0_data = SYNC_BYTE;
enum data uart1_data = SYNC_BYTE;
enum data uart2_data = SYNC_BYTE;

uint8_t len[3];

typedef struct {
	uint8_t buffer[BUF_SIZE];
	volatile unsigned int producer;
	volatile unsigned int consumer;
} Buffer_t;

static Buffer_t buf_u0;
static Buffer_t buf_u1;
static Buffer_t buf_u2;
static Buffer_t *buf;

static bool full_buffer(Buffer_t * buffer) {
	return (((buffer->producer + 1) % BUF_SIZE) == buffer->consumer);
}

static bool empty_buffer(Buffer_t * buffer) {
	return ((buffer->producer) == buffer->consumer);
}

void state(enum data *p_data, LPC_USART_T *called_uart, Buffer_t *called_buf, uint8_t *lenght) {
	uint32_t pom;
	int called_u;
	if(called_uart == UART0)
		called_u = 0;
	else if(called_uart == UART1)
		called_u = 1;
	else if(called_uart == UART2)
		called_u = 2;
	while (!(UART_IIR_INTSTAT_PEND & (pom = Chip_UART_ReadIntIDReg(called_uart)))) {
		pom = (pom & 0xE) >> 1;
		switch (pom) {
		//THRE
		case 0x1: {
			//kdyz je buffer uartun prazdny
			if (empty_buffer(called_buf)) {
				Chip_UART_IntDisable(called_uart, UART_IER_THREINT);
				break;
			}
			int i = FIFO_SIZE;
			//do te doby nez se vyprazdni buffer pro zapis a nepretece fifo
			while (!empty_buffer(called_buf) && i > 0) {
				Chip_UART_SendByte(called_uart,called_buf->buffer[called_buf->consumer]);
				called_buf->consumer = (called_buf->consumer + 1)% BUF_SIZE;
				i--;
			}
			break;
		}
		//RDA a CTI
		case 0x6:

		case 0x2: {
			uint8_t x;
			int fs = FIFO_SIZE;
			// dokud bude rx fifo uartu obsahovat nejake data
			while (UART_LSR_RDR & Chip_UART_ReadLineStatus(called_uart)) {

				switch (*p_data) {
				case SYNC_BYTE: {
					x = Chip_UART_ReadByte(called_uart);
					if (x == 170){
						*p_data = DEST_UART;
					}
					else {
						break;
					}
					break;
				}
				case DEST_UART: {
					x = Chip_UART_ReadByte(called_uart);
					if (x == 0 && called_uart != UART0) {
						write_uart[called_u] = UART0;
						buf = &buf_u0;
						*p_data = LENGHT_OF_DATA;
					}
					else if (x == 1 && called_uart != UART1) {
						write_uart[called_u] = UART1;
						buf = &buf_u1;
						*p_data = LENGHT_OF_DATA;
					}
					else if (x == 2 && called_uart != UART2) {
						write_uart[called_u] = UART2;
						buf = &buf_u2;
						*p_data = LENGHT_OF_DATA;
					}
					else if (x == 170) {
						*p_data = DEST_UART;
					}
					break;
				}
				case LENGHT_OF_DATA: {
					x = Chip_UART_ReadByte(called_uart);
					if(x > 0 && x < 65){
						*lenght = x;
						*p_data = RECEIVE_DATA;
						Chip_UART_SendByte(write_uart[called_u], 170);
						fs--;
						Chip_UART_SendByte(write_uart[called_u], called_u);
						fs--;
						Chip_UART_SendByte(write_uart[called_u], *lenght);
						fs--;
					}
					else if (x == 170)
						*p_data = DEST_UART;
					break;
				}
				case RECEIVE_DATA: {
					if (empty_buffer(called_buf) && (UART_LSR_THRE & Chip_UART_ReadLineStatus(write_uart[called_u]))) {
						while (UART_LSR_RDR & Chip_UART_ReadLineStatus(called_uart)) {
							x = Chip_UART_ReadByte(called_uart);
							if (x == 170) {
								*p_data = DEST_UART;
								break;
							}
							if (x != '0' && *lenght > 0) {
								if (fs > 0) {
									Chip_UART_SendByte(write_uart[called_u], x);
									*lenght = *lenght - 1;
									fs--;
								} else {
									if (!full_buffer(buf)) {
										buf->buffer[buf->producer] = x;
										*lenght = *lenght - 1;
										buf->producer = (buf->producer + 1) % BUF_SIZE;
									}
								}
							}
						}
					} else {
						while ((!full_buffer(called_buf)) && (UART_LSR_RDR & Chip_UART_ReadLineStatus(called_uart))) {
							x = Chip_UART_ReadByte(called_uart);
							if (x == 170) {
								*p_data = DEST_UART;
								break;
							}
							if (x != '0' && lenght > 0) {
								buf->buffer[buf->producer] = x;
								*lenght = *lenght - 1;
								buf->producer = (buf->producer + 1)% BUF_SIZE;
							}
						}
					}
					Chip_UART_IntEnable(write_uart[called_u], UART_IER_THREINT);
					break;
				}
				}
			}
		}
		}
	}
}
/*
void __lpc1788_isr_uart0(void) {
	state(&uart0_data, UART0, &buf_u0,&len[0]);
}
void __lpc1788_isr_uart1(void) {
	state(&uart1_data, UART1, &buf_u1,&len[1]);
}
void __lpc1788_isr_uart2(void) {
	state(&uart2_data, UART2, &buf_u2,&len[2]);
}
*/
void serial_init(void){

	// Inicializace hodin pro blok iocon, gpio
	Chip_IOCON_Init(LPC_IOCON);
	Chip_GPIO_Init(LPC_GPIO);
	//Initialize - pin connect
	//uart0
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 2, (IOCON_FUNC1 | IOCON_MODE_INACT));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 3, (IOCON_FUNC1 | IOCON_MODE_INACT));
	//uart1
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 15, (IOCON_FUNC1 | IOCON_MODE_INACT));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 16, (IOCON_FUNC1 | IOCON_MODE_INACT));
	//uart2
	Chip_IOCON_PinMuxSet(LPC_IOCON, 2, 8, (IOCON_FUNC2 | IOCON_MODE_INACT));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 2, 9, (IOCON_FUNC2 | IOCON_MODE_INACT));
	//led
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 13, (IOCON_FUNC0 | IOCON_MODE_INACT));
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 13);
	Chip_GPIO_SetPinOutLow(LPC_GPIO, 0, 13);

	Chip_UART_Init(UART0);
	Chip_UART_SetBaud(UART0, 115200);
	Chip_UART_ConfigData(UART0, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(UART0, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(UART0);
	Chip_UART_IntEnable(UART0, (UART_IER_RBRINT));

	Chip_UART_Init(UART1);
	Chip_UART_SetBaud(UART1, 115200);
	Chip_UART_ConfigData(UART1, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(UART1, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(UART1);
	Chip_UART_IntEnable(UART1, (UART_IER_RBRINT));

	Chip_UART_Init(UART2);
	Chip_UART_SetBaud(UART2, 9600);
	Chip_UART_ConfigData(UART2, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(UART2, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(UART2);
	Chip_UART_IntEnable(UART2, (UART_IER_RBRINT));

	Chip_UART_SetupFIFOS(UART0, (UART_FCR_FIFO_EN | UART_FCR_RX_RS |
	UART_FCR_TX_RS | UART_FCR_TRG_LEV3));
	Chip_UART_SetupFIFOS(UART1, (UART_FCR_FIFO_EN | UART_FCR_RX_RS |
	UART_FCR_TX_RS | UART_FCR_TRG_LEV3));
	Chip_UART_SetupFIFOS(UART2, (UART_FCR_FIFO_EN | UART_FCR_RX_RS |
	UART_FCR_TX_RS | UART_FCR_TRG_LEV3));

	NVIC_EnableIRQ(UART0_IRQn);
	NVIC_EnableIRQ(UART1_IRQn);
	NVIC_EnableIRQ(UART2_IRQn);

}
