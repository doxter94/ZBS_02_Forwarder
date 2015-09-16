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

bool UDP_packet_send(char * data, uint8_t length,char destination_addr[6],
		char source_addr[6],char source_ip[4],char destination_ip[4]);

#endif /* ETHERNET_H_ */
