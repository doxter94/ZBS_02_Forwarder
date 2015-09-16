#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "chip.h"
#include "ethernet.h"

/*****************************************************************************
 * Defines
 ****************************************************************************/

#define UART0 (LPC_UART0)
#define UART2 (LPC_UART2)
#define UART4 (LPC_UART4)
#define BLUE_LED_GPIO_PORT (2)
#define BLUE_LED_GPIO_PIN (3)
#define RED_LED_GPIO_PORT (2)
#define RED_LED_GPIO_PIN (4)

#define LAST_CHAR	0x0A
#define FIRST_CHAR	0x23
#define CONFIG_CHAR	0x43
#define MAX_LENGHT	254
#define PROTOCOL_LENGTH	25
/* Page used for storage */
#define PAGE_ADDR	0x01
#define BUF_CONSUMER_PLUS1 (buf_uart.consumer = (buf_uart.consumer + 1)% MAX_LENGHT)
#define BUF_PRODUCER_PLUS1 (buf_uart.producer = (buf_uart.producer + 1)% MAX_LENGHT)

/*****************************************************************************
 * Variables
 ****************************************************************************/

static uint8_t read_byte;

static uint8_t buffer[MAX_LENGHT];

static uint8_t eeprom_buffer[EEPROM_PAGE_SIZE];

static uint8_t eeprom_read[EEPROM_PAGE_SIZE];

static uint8_t message_lenght;

static uint8_t eeprom_lenght;

static uint8_t count;

/* Destination MAC addr */
static char destination_addr[6] = "\x00\x1e\x0b\x3e\xe3\xb3";
/* Source MAC addr */
static char source_addr[6] = "\x6c\x62\x7d\x8a\x53\x3e";
/* Source ip */
static char source_ip[4] = "\xc0\xa8\x01\x01";
/* Destination ip */
static char destination_ip[4] = "\xc0\xa8\x00\xd0";


/*****************************************************************************
 * Private functions
 ****************************************************************************/

enum transmit_state{
	WAIT_FOR_DATA,
	WRONG_MESSAGE,
	RIGHT_MESSAGE,
	ETHERNET_TRANSMIT
};

static enum transmit_state state = WAIT_FOR_DATA;


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

/* Zápis do paměti eeprom podle určeného protokolu. */
void eeprom_write(){
	while(eeprom_lenght >= PROTOCOL_LENGTH){
		if(eeprom_buffer[count] == CONFIG_CHAR){
			Chip_EEPROM_Write(LPC_EEPROM, 0, PAGE_ADDR, eeprom_buffer + count, EEPROM_RWSIZE_8BITS, eeprom_lenght - count);
			memset(eeprom_buffer,0,eeprom_lenght); // Vynulování dat v bufferu
			eeprom_lenght = 0;
			count = 0;
		} else{
			count++;
		}
	}
}

/*****************************************************************************
 * Initialization
 ****************************************************************************/

/* Init EEPROM */
void eeprom_init(){

	Chip_EEPROM_Init(LPC_EEPROM);

}

/* Init seriové linky a pinů určených k přenosu dat
 * Nastavení UARTu: 115.2K8N1.
 *  FIFO lvl3 - 14 znaků.
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

/* Přerušení..
 * UART0 - Konfigurace mac/ip adresy
 * UART2 - Pro konektory J4 - 5 na desce
 * UART4 - Pro debug test (konektory J6 - 7)
 *
 * Přijímání dat po bajtu od zařízení JN5148 a ukládání
 * do kruhového bufferu.
 */
void __lpc1788_isr_uart0(void) {
	while (UART_LSR_RDR & Chip_UART_ReadLineStatus(UART0)) {
		eeprom_buffer[eeprom_lenght] = Chip_UART_ReadByte(UART0);
		eeprom_lenght++;
	}
}

void __lpc1788_isr_uart2(void) {
	while (UART_LSR_RDR & Chip_UART_ReadLineStatus(UART2)) {
		read_byte = Chip_UART_ReadByte(UART2);
		buf_uart.buffer[buf_uart.producer] = read_byte;
		BUF_PRODUCER_PLUS1;
	}
}

void __lpc1788_isr_uart4(void) {
	while (UART_LSR_RDR & Chip_UART_ReadLineStatus(LPC_UART4)) {
		read_byte = Chip_UART_ReadByte(LPC_UART4);
		buf_uart.buffer[buf_uart.producer] = read_byte;
		BUF_PRODUCER_PLUS1;
	}
}

/* Přeposílání jednotlivých zpráv na ethernet
 *
 * Funkce filtruje zprávy, které začínaji znakem #.
 * Každá přijatá zpráva ukončená znakem 0x0A se odesílá
 * jednotlivě se zpožděním kvůli synchronizaci přenosu.
 * Před každým odesláním packetu prečte data z paměti.
 *  */
void ethernet_transmit(){
		switch (state) {
			case WAIT_FOR_DATA: {
				while(!empty_buffer(&buf_uart)){
					if(buf_uart.buffer[buf_uart.consumer] == FIRST_CHAR){
						BUF_CONSUMER_PLUS1;
						state = WRONG_MESSAGE;
						break;
					}
					if(buf_uart.buffer[buf_uart.consumer] == LAST_CHAR){
						BUF_CONSUMER_PLUS1;
						break;
					}
					state = RIGHT_MESSAGE;
					break;
				}
				break;
			}
			case WRONG_MESSAGE: {
				while(!empty_buffer(&buf_uart)){
					if(buf_uart.buffer[buf_uart.consumer] != LAST_CHAR){
						BUF_CONSUMER_PLUS1;
					} else{
						BUF_CONSUMER_PLUS1;
						state = WAIT_FOR_DATA;
						break;
					}
				}
				break;
			}
			case RIGHT_MESSAGE: {
				while(!empty_buffer(&buf_uart)){
					if(buf_uart.buffer[buf_uart.consumer] == FIRST_CHAR){
						state = WRONG_MESSAGE;
						break;
					}
					if(buf_uart.buffer[buf_uart.consumer] != LAST_CHAR){
						buffer[message_lenght] = buf_uart.buffer[buf_uart.consumer];
						BUF_CONSUMER_PLUS1;
						message_lenght++;
					} else{
						buffer[message_lenght] = buf_uart.buffer[buf_uart.consumer];
						BUF_CONSUMER_PLUS1;
						message_lenght++;
						state = ETHERNET_TRANSMIT;
						break;
					}
				}
				break;
			}
			case ETHERNET_TRANSMIT: {

				/* Read all data from EEPROM page*/
				Chip_EEPROM_Read(LPC_EEPROM, 0, PAGE_ADDR, eeprom_read, EEPROM_RWSIZE_8BITS, EEPROM_PAGE_SIZE);

				/* Destination address */
				memcpy(destination_addr,(char*)eeprom_read + 2, sizeof(destination_addr));
				/* Source address */
				memcpy(source_addr,(char*)eeprom_read + 9, sizeof(source_addr));
				/* Source ip */
				memcpy(source_ip,(char*)eeprom_read + 16, sizeof(source_ip));
				/* Destination ip */
				memcpy(destination_ip,(char*)eeprom_read + 21, sizeof(destination_ip));

				delayMs(3);
				UDP_packet_send((char*) buffer, message_lenght,destination_addr,
						source_addr,source_ip,destination_ip);

				memset(buffer,0,message_lenght); // Vynulování přeposlaných dat v bufferu
				message_lenght = 0;
				state = WAIT_FOR_DATA;
				break;
			}

		}
}
