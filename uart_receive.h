#ifndef UART_RECEIVE_H_
#define UART_RECEIVE_H_

void setup_uarts();

void ethernet_transmit();

void uart_debug(const char * text, LPC_USART_T *pUART);

#endif /* UART_RECEIVE_H_ */
