/** @file Doxter_lib.h*/
/*
 * Doxter_lib.h
 *
 *  Created on: Jun 29, 2015
 *      Author: jirka
 *
 *  Info: This is mine library which contains useful functions
 *
 */

#ifndef DOXTER_LIB_H_
#define DOXTER_LIB_H_

/*****************************************************************************
 * Private functions (for example)
 ****************************************************************************/

void D_uart_write(const char * text, LPC_USART_T *pUART){
	uint16_t numBytes = 0;
	const char * text_iterace = text;

	while (*text_iterace++) {
		numBytes++;
	}

	Chip_UART_SendBlocking(pUART, text, numBytes);
}



#endif /* DOXTER_LIB_H_ */
