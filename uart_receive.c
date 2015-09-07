#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "chip.h"
#include "ethernet.h"

#define UART0 (LPC_UART0)
#define UART2 (LPC_UART2)
#define UART4 (LPC_UART4)

#define BLUE_LED_GPIO_PORT (2)
#define BLUE_LED_GPIO_PIN (3)
#define RED_LED_GPIO_PORT (2)
#define RED_LED_GPIO_PIN (4)
#define LAST_CHAR 0x0A
#define MAX_LENGHT 127

/*****************************************************************************
 * Variables
 ****************************************************************************/

static uint8_t read_byte;

static uint8_t buffer[MAX_LENGHT];

static uint8_t message_lenght;

uint32_t curent_byte;

/*****************************************************************************
 * Private functions
 ****************************************************************************/

typedef struct {
	uint8_t buffer[MAX_LENGHT];
	volatile unsigned int producer;
	volatile unsigned int consumer;
} Buffer_t;

static Buffer_t buf_uart;

static void delay(uint32_t i);

/* Delay in miliseconds  (cclk = 120MHz) */
static void delayMs(uint32_t ms) {
	delay(ms * 30030);
}

static bool full_buffer(Buffer_t * buffer) {
	return (((buffer->producer + 1) % MAX_LENGHT) == buffer->consumer);
}

static bool empty_buffer(Buffer_t * buffer) {
	return ((buffer->producer) == buffer->consumer);
}

void uart_debug(const char * text, LPC_USART_T *pUART){
	uint16_t numBytes = 0;
	const char * text_iterace = text;

	while (*text_iterace++) {
		numBytes++;
	}

	Chip_UART_SendBlocking(pUART, text, numBytes);
}

/*****************************************************************************
 * Initialization
 ****************************************************************************/

/* Init seriové linky a pinů určených k přenosu dat
 * Nastavení UARTu: 115.2K8N1
 *  FIFO lvl3 - 14 znaků
 *  */
void setup_uarts(){

	/* UART0 set up */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 2, (IOCON_FUNC1 | IOCON_MODE_INACT));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 3, (IOCON_FUNC1 | IOCON_MODE_INACT));

	/* Setup UART0 for 115.2K8N1 */
	Chip_UART_Init(LPC_UART0);
	Chip_UART_SetBaud(LPC_UART0, 115200);
	Chip_UART_ConfigData(LPC_UART0, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(LPC_UART0, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(LPC_UART0);
	Chip_UART_IntEnable(UART0, (UART_IER_RBRINT));
	Chip_UART_SetupFIFOS(LPC_UART0, (UART_FCR_FIFO_EN | UART_FCR_RX_RS |
	UART_FCR_TX_RS | UART_FCR_TRG_LEV3));

	NVIC_EnableIRQ(UART0_IRQn);

	/* UART2 set up */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 2, 8, (IOCON_FUNC2 | IOCON_MODE_INACT));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 2, 9, (IOCON_FUNC2 | IOCON_MODE_INACT));

	/* Setup UART4 for 115.2K8N1 */
	Chip_UART_Init(UART2);
	Chip_UART_SetBaudFDR(UART2, 115200);
	Chip_UART_ConfigData(UART2, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT | UART_LCR_PARITY_DIS));
	Chip_UART_TXEnable(UART2);
	Chip_UART_IntEnable(UART2, (UART_IER_RBRINT));
	Chip_UART_SetupFIFOS(UART2, (UART_FCR_FIFO_EN | UART_FCR_RX_RS |
	UART_FCR_TX_RS | UART_FCR_TRG_LEV3));

	NVIC_EnableIRQ(UART2_IRQn);

	/* UART4 set up */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 22, (IOCON_FUNC3 | IOCON_MODE_INACT));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 5, 3, (IOCON_FUNC4 | IOCON_MODE_INACT));


	/* Setup UART4 for 115.2K8N1 */
	Chip_UART_Init(LPC_UART4);
	Chip_UART_SetBaudFDR(LPC_UART4, 115200);
	Chip_UART_ConfigData(LPC_UART4, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT | UART_LCR_PARITY_DIS));
	Chip_UART_TXEnable(LPC_UART4);
	Chip_UART_IntEnable(LPC_UART4, (UART_IER_RBRINT));
	Chip_UART_SetupFIFOS(LPC_UART4, (UART_FCR_FIFO_EN | UART_FCR_RX_RS |
	UART_FCR_TX_RS | UART_FCR_TRG_LEV3));

	//Chip_UART_SetAutoBaudReg(LPC_UART4,UART_ACR_START | UART_ACR_MODE);

	NVIC_EnableIRQ(UART4_IRQn);

}

/*****************************************************************************
 * Main functions
 ****************************************************************************/

/*
 * UART0 - Pro test UARTu0
 * UART2 - Pro konektory J4 - 5 na desce
 * UART4 - Pro debug test (konektory J6 - 7)
 *
 * Přijímání dat po bajtu od zařízení jennic a ukládání
 * do kruhového bufferu
 */
void __lpc1788_isr_uart0(void) {

	while (UART_LSR_RDR & Chip_UART_ReadLineStatus(UART0)) {
		read_byte = Chip_UART_ReadByte(UART0);
		buf_uart.buffer[buf_uart.producer] = read_byte;
		buf_uart.producer = (buf_uart.producer + 1)% MAX_LENGHT;
	}
}

void __lpc1788_isr_uart2(void) {

	while (UART_LSR_RDR & Chip_UART_ReadLineStatus(UART2)) {
		read_byte = Chip_UART_ReadByte(UART2);
		buf_uart.buffer[buf_uart.producer] = read_byte;
		buf_uart.producer = (buf_uart.producer + 1)% MAX_LENGHT;
	}
}

void __lpc1788_isr_uart4(void) {
	while (UART_LSR_RDR & Chip_UART_ReadLineStatus(LPC_UART4)) {
		read_byte = Chip_UART_ReadByte(LPC_UART4);
		buf_uart.buffer[buf_uart.producer] = read_byte;
		buf_uart.producer = (buf_uart.producer + 1)% MAX_LENGHT;
	}
}

/* Přeposílání jednotlivých zpráv na ethernet
 *
 * Každá přijatá zpráva ukončená znakem 0x0A
 * se odesílá jednotlivě se zpožděním
 * kvůli synchronizaci přenosu
 *  */
void ethernet_transmit(){
	while(!empty_buffer(&buf_uart)){
		/* Kontrola dat v bufferu */
		if(buf_uart.buffer[buf_uart.consumer] == LAST_CHAR){
			delayMs(3);
			UDP_packet_send((char*) buffer, message_lenght);
			memset(buffer,0,message_lenght); // Vynulování přeposlaných dat v bufferu
			message_lenght = 0;
			buf_uart.consumer = (buf_uart.consumer + 1)% MAX_LENGHT;

		} else{
			buffer[message_lenght] = buf_uart.buffer[buf_uart.consumer];
			buf_uart.consumer = (buf_uart.consumer + 1)% MAX_LENGHT;
			message_lenght++;
		}
	}
}
