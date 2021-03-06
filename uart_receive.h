#ifndef UART_RECEIVE_H_
#define UART_RECEIVE_H_

void setup_uarts();

void eeprom_init();

void ethernet_transmit();

void eeprom_write();

void uart_debug(const char * text, LPC_USART_T *pUART);

#endif /* UART_RECEIVE_H_ */
