/*
 * ethernet.h
 *
 *  Created on: Jul 15, 2015
 *      Author: jirka
 */

#ifndef ETHERNET_H_
#define ETHERNET_H_

void ethernet(void);

void enet_setup();

void enet_init();

void UDP_packet_send(char * data, uint8_t length);

#endif /* ETHERNET_H_ */
