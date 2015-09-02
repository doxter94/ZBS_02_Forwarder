#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "chip.h"
#include "ethernet.h"

#define UART0 (LPC_UART0)
#define UART1 (LPC_UART1)
#define UART2 (LPC_UART2)
#define UART3 (LPC_UART3)

#define BLUE_LED_GPIO_PORT (2)
#define BLUE_LED_GPIO_PIN (3)
#define RED_LED_GPIO_PORT (2)
#define RED_LED_GPIO_PIN (4)
#define MAX_LENGHT 127
#define LAST_CHAR 0x0A

/*****************************************************************************
 * Variables
 ****************************************************************************/

static uint8_t read_byte;

static uint8_t buffer[MAX_LENGHT];

static uint8_t message_lenght;

static uint8_t last_byte = LAST_CHAR;

/*****************************************************************************
 * Private functions
 ****************************************************************************/


typedef struct {
	uint8_t buffer[MAX_LENGHT];
	volatile unsigned int producer;
	volatile unsigned int consumer;
} Buffer_t;

static Buffer_t buf_u2;

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

static void blue(int delay){
	Chip_GPIO_SetPinOutHigh(LPC_GPIO, 2, 3);
	delayMs(delay);
	Chip_GPIO_SetPinOutLow(LPC_GPIO, 2, 3);
	delayMs(delay);
}

static void uart_debug(const char * text, LPC_USART_T *pUART){
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

	/* Setup UART2 for 115.2K8N1 */
	Chip_UART_Init(LPC_UART2);
	Chip_UART_SetBaud(LPC_UART2, 115200);
	Chip_UART_ConfigData(LPC_UART2, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(LPC_UART2, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(LPC_UART2);
	Chip_UART_IntEnable(LPC_UART2, (UART_IER_RBRINT));
	Chip_UART_SetupFIFOS(LPC_UART2, (UART_FCR_FIFO_EN | UART_FCR_RX_RS |
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

/* UART0 slouzi pro test */
void __lpc1788_isr_uart0(void) {
	uint32_t pom;
	while (!(UART_IIR_INTSTAT_PEND & (pom = Chip_UART_ReadIntIDReg(LPC_UART0)))) {
		pom = (pom & 0xE) >> 1;
		switch (pom) {

		//RDA a CTI
		case 0x6:
		case 0x2: {
			/* When the UART receiver FIFO is not empty */
			while (UART_LSR_RDR & Chip_UART_ReadLineStatus(LPC_UART0)){

				if(last_byte != LAST_CHAR){
					buf_u2.buffer[buf_u2.producer] = last_byte;
					buf_u2.producer = (buf_u2.producer + 1)% MAX_LENGHT;
					message_lenght++;
					last_byte = LAST_CHAR;
				}
				read_byte = Chip_UART_ReadByte(LPC_UART0);
				if(read_byte != LAST_CHAR && message_lenght != MAX_LENGHT){

					buf_u2.buffer[buf_u2.producer] = read_byte;
					buf_u2.producer = (buf_u2.producer + 1)% MAX_LENGHT;
					message_lenght++;

				} else{

					uint8_t i;
					for(i = 0;i < message_lenght; i++){
						buffer[i] = buf_u2.buffer[buf_u2.consumer];
						buf_u2.consumer = (buf_u2.consumer + 1)% MAX_LENGHT;
					}

					if(read_byte != LAST_CHAR)
						last_byte = read_byte;

					UDP_packet_send((char*)buffer,message_lenght);
					memset(buffer, 0 , sizeof(buffer));
					message_lenght = 0;

				}
			}
		}
		}
	}
}

void __lpc1788_isr_uart2(void) {
	uint32_t pom;
	while (!(UART_IIR_INTSTAT_PEND & (pom = Chip_UART_ReadIntIDReg(LPC_UART2)))) {
		pom = (pom & 0xE) >> 1;
		switch (pom) {

		//RDA a CTI
		case 0x6:
		case 0x2: {
			/* When the UART receiver FIFO is not empty */
			while (UART_LSR_RDR & Chip_UART_ReadLineStatus(LPC_UART2)){

				if(last_byte != LAST_CHAR){
					buf_u2.buffer[buf_u2.producer] = last_byte;
					buf_u2.producer = (buf_u2.producer + 1)% MAX_LENGHT;
					message_lenght++;
					last_byte = LAST_CHAR;
				}
				read_byte = Chip_UART_ReadByte(LPC_UART2);
				if(read_byte != LAST_CHAR && message_lenght != MAX_LENGHT){

					buf_u2.buffer[buf_u2.producer] = read_byte;
					buf_u2.producer = (buf_u2.producer + 1)% MAX_LENGHT;
					message_lenght++;

				} else{

					uint8_t i;
					for(i = 0;i < message_lenght; i++){
						buffer[i] = buf_u2.buffer[buf_u2.consumer];
						buf_u2.consumer = (buf_u2.consumer + 1)% MAX_LENGHT;
					}

					if(read_byte != LAST_CHAR)
						last_byte = read_byte;

					UDP_packet_send((char*)buffer,message_lenght);
					memset(buffer, 0 , sizeof(buffer));
					message_lenght = 0;

				}
			}
		}
		}
	}
}

/* test */
void __lpc1788_isr_uart4(void) {

	while (UART_LSR_RDR & Chip_UART_ReadLineStatus(LPC_UART4)){
		read_byte = Chip_UART_ReadByte(LPC_UART4);

		if(read_byte != LAST_CHAR && message_lenght != MAX_LENGHT){

			buf_u2.buffer[buf_u2.producer] = read_byte;
			buf_u2.producer = (buf_u2.producer + 1)% MAX_LENGHT;
			message_lenght++;
		}
		else{
			uint8_t i;
			for(i = 0;i < message_lenght; i++){
				buffer[i] = buf_u2.buffer[buf_u2.consumer];
				buf_u2.consumer = (buf_u2.consumer + 1)% MAX_LENGHT;
			}
			delayMs(5);
			uart_debug((char*)buffer,LPC_UART0);
			uart_debug("\r\n",LPC_UART0);
			delayMs(5);
			UDP_packet_send((char*)buffer,message_lenght);
			memset(buffer, 0 , sizeof(buffer));
			message_lenght = 0;
		}
	}
}
