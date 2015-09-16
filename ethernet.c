/*
 * ethernet.c
 *
 *  Created on: Jan 29, 2015
 *      Author: wella
 */

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "chip.h"
#include "lpc_phy.h"

#define BUFFER_SIZE 170
#define HEADER_SIZE 20


static uint16_t identification;

/*****************************************************************************
 * Private functions
 ****************************************************************************/
void delay(uint32_t i);
/* Delay in miliseconds  (cclk = 120MHz) */
static void delayMs(uint32_t ms) {
	delay(ms * 30030);
}

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

#define ENET_NUM_TX_DESC 4
#define ENET_NUM_RX_DESC 4

#if defined(CHIP_LPC175X_6X)
#define ENET_RX_DESC_BASE        (0x2007C000)
#else
/* The Ethernet Block can only access Peripheral SRAM and External Memory. In this example,
 Peripheral SRAM is selected for storing descriptors, status arrays and send/receive buffers.*/
#define ENET_RX_DESC_BASE        (0x20000000UL)
#endif
#define ENET_RX_STAT_BASE        (ENET_RX_DESC_BASE + ENET_NUM_RX_DESC * sizeof(ENET_RXDESC_T))
#define ENET_TX_DESC_BASE        (ENET_RX_STAT_BASE + ENET_NUM_RX_DESC * sizeof(ENET_RXSTAT_T))
#define ENET_TX_STAT_BASE        (ENET_TX_DESC_BASE + ENET_NUM_TX_DESC * sizeof(ENET_TXDESC_T))
#define ENET_RX_BUF_BASE         (ENET_TX_STAT_BASE + ENET_NUM_TX_DESC * sizeof(ENET_TXSTAT_T))
#define ENET_TX_BUF_BASE         (ENET_RX_BUF_BASE  + ENET_NUM_RX_DESC * ENET_ETH_MAX_FLEN)
#define ENET_RX_BUF(i)           (ENET_RX_BUF_BASE + ENET_ETH_MAX_FLEN * i)
#define ENET_TX_BUF(i)           (ENET_TX_BUF_BASE + ENET_ETH_MAX_FLEN * i)

STATIC ENET_RXDESC_T *pRXDescs = (ENET_RXDESC_T *) ENET_RX_DESC_BASE;
STATIC ENET_RXSTAT_T *pRXStats = (ENET_RXSTAT_T *) ENET_RX_STAT_BASE;
STATIC ENET_TXDESC_T *pTXDescs = (ENET_TXDESC_T *) ENET_TX_DESC_BASE;
STATIC ENET_TXSTAT_T *pTXStats = (ENET_TXSTAT_T *) ENET_TX_STAT_BASE;

/* Transmit/receive buffers and indices */
STATIC int32_t rxConsumeIdx;
STATIC int32_t txProduceIdx;

/* Initialize MAC descriptors for simple packet receive/transmit */
STATIC void InitDescriptors(void) {
	int i;

	/* Setup the descriptor list to a default state */
	memset(pTXDescs, 0, ENET_NUM_TX_DESC * sizeof(ENET_TXDESC_T));
	memset(pTXStats, 0, ENET_NUM_TX_DESC * sizeof(ENET_TXSTAT_T));
	memset(pRXDescs, 0, ENET_NUM_RX_DESC * sizeof(ENET_RXDESC_T));
	memset(pRXStats, 0, ENET_NUM_RX_DESC * sizeof(ENET_RXSTAT_T));

	rxConsumeIdx = 0;
	rxConsumeIdx = 0;

	/* Build linked list, CPU is owner of descriptors */
	for (i = 0; i < ENET_NUM_RX_DESC; i++) {
		pRXDescs[i].Packet = (uint32_t) ENET_RX_BUF(i);
		pRXDescs[i].Control = ENET_RCTRL_SIZE(ENET_ETH_MAX_FLEN);
		pRXStats[i].StatusInfo = 0;
		pRXStats[i].StatusHashCRC = 0;
	}
	for (i = 0; i < ENET_NUM_TX_DESC; i++) {
		pTXDescs[i].Packet = (uint32_t) ENET_TX_BUF(i);
		pTXDescs[i].Control = 0;
		pTXStats[i].StatusInfo = 0;
	}

	/* Setup list pointers in Ethernet controller */
	Chip_ENET_InitTxDescriptors(LPC_ETHERNET, pTXDescs, pTXStats,
	ENET_NUM_TX_DESC);
	Chip_ENET_InitRxDescriptors(LPC_ETHERNET, pRXDescs, pRXStats,
	ENET_NUM_RX_DESC);
}

/* Get the pointer to the Rx buffer storing new received frame */
STATIC void *ENET_RXGet(int32_t *bytes) {
	uint16_t produceIdx;
	void *buffer;

	produceIdx = Chip_ENET_GetRXProduceIndex(LPC_ETHERNET);
	/* This doesn't check status of the received packet */
	if (Chip_ENET_GetBufferStatus(LPC_ETHERNET, produceIdx, rxConsumeIdx,
	ENET_NUM_RX_DESC) != ENET_BUFF_EMPTY) {
		/* CPU owns descriptor, so a packet was received */
		buffer = (void *) pRXDescs[rxConsumeIdx].Packet;
		*bytes = (int32_t) (ENET_RINFO_SIZE(pRXStats[rxConsumeIdx].StatusInfo)
				- 4);/* Remove CRC */
	} else {
		/* Nothing received */
		*bytes = 0;
		buffer = NULL;
	}

	return buffer;
}

/* Release Rx Buffer */
STATIC void ENET_RXBuffClaim(void) {
	rxConsumeIdx = Chip_ENET_IncRXConsumeIndex(LPC_ETHERNET);
}

/* Get Tx Buffer for the next transmission */
STATIC void *ENET_TXBuffGet(void) {
	uint16_t consumeIdx = Chip_ENET_GetTXConsumeIndex(LPC_ETHERNET);

	if (Chip_ENET_GetBufferStatus(LPC_ETHERNET, txProduceIdx, consumeIdx,
	ENET_NUM_TX_DESC) != ENET_BUFF_FULL) {
		return (void *) pTXDescs[txProduceIdx].Packet;
	}
	return NULL;
}

/* Queue a new frame for transmission */
STATIC void ENET_TXQueue(int32_t bytes) {
	if (bytes > 0) {
		pTXDescs[txProduceIdx].Control = ENET_TCTRL_SIZE(
				bytes) | ENET_TCTRL_LAST;
		txProduceIdx = Chip_ENET_IncTXProduceIndex(LPC_ETHERNET);
	}
}

/* Check if tranmission finished */
STATIC bool ENET_IsTXFinish(void) {
	uint16_t consumeIdx = Chip_ENET_GetTXConsumeIndex(LPC_ETHERNET);

	if (Chip_ENET_GetBufferStatus(LPC_ETHERNET, txProduceIdx, consumeIdx,
	ENET_NUM_TX_DESC) == ENET_BUFF_EMPTY) {
		return true;
	}
	return false;
}


void enet_setup(){
	/* Setup MII clock rate and PHY address */
	Chip_ENET_SetupMII(LPC_ETHERNET,
			Chip_ENET_FindMIIDiv(LPC_ETHERNET, 2500000), 0);

	lpc_phy_init(true, delayMs);

	/* Setup descriptors */
	InitDescriptors();

	/* Enable RX/TX after descriptors are setup */
	Chip_ENET_TXEnable(LPC_ETHERNET);
	Chip_ENET_RXEnable(LPC_ETHERNET);
}


/* Vypočítání IP checksumu
 * Parametry:
 * 	buf - pole obsahující IP hlavičku ke zpracování
 * 	size - velikost přijatých dat
 * */
uint16_t ip_header_checksum(const uint8_t *buf, unsigned size)
{
	 uint8_t p[size];
	 int c;
	 for (c = 0; c < size; c++)
		 p[c] = buf[c];


 	/* Accumulate checksum */
	unsigned sum = 0;
	uint8_t i;
 	for (i = 0; i < size - 1; i += 2)
 	{
 		unsigned short word16 = 256U*p[i]+p[i+1];
 		sum += word16;
 	}

   sum = (sum >> 16) + (sum & 0xffff);
   sum += (sum >> 16);
   sum =  ~sum;
   return (uint16_t)sum;
}

/* Calculate the UDP header checksum */
uint16_t udp_header_checksum(const uint16_t source_ip, const uint16_t destination_ip,
		const uint16_t lenght, const uint8_t *data, unsigned data_size)
{
 	/* Accumulate checksum */
	unsigned sum = 0;

	 uint8_t pole[data_size];
	 int c;
	 for (c = 0; c < data_size; c++)
		 pole[c] = data[c];

	int i;
 	for (i = 0; i < data_size - 1; i += 2)
 	{
 		unsigned short word16 = 256U*pole[i]+pole[i+1];
 		sum += word16;
 	}

   sum = (sum >> 16) + (sum & 0xffff);
   sum += (sum >> 16);
   sum =  ~sum;
   return (uint16_t)sum;
}


/* Poslání UDP packetu
 * Parametry:
 * 	data - pole obsahující data k odeslání
 * 	length - délka přijatých dat
 *  */
bool UDP_packet_send(char * data, uint8_t length,char destination_addr[6],
		char source_addr[6],char source_ip[4],char destination_ip[4]){
	static uint16_t ip_checksum; //calc later
	//static uint16_t udp_checksum; //calc later
	const uint8_t total_length = length + 28;
	const uint8_t udp_length = length + 8;

	//transmit buf
	static uint8_t *buffer;

	buffer = ENET_TXBuffGet();

	memcpy(buffer,destination_addr,6); //Destination MAC (0 - 5)

	memcpy(buffer + 6,source_addr,6); //Source MAC (6 - 11)

	memcpy(buffer + 12,"\x08\x00",2); //Type: IP (12 - 13)

	memcpy(buffer + 14,"\x45\x00",2); //Version and header length (14 - 15)

	//total length (16 - 17)
	memcpy(buffer + 16,"\x00",1);
	snprintf((char*)buffer + 17,sizeof(buffer),"%c",total_length);

	// identification (18 - 19)
	const uint8_t id_part2 = identification >> 8;
	const uint8_t id_part1 = identification;

	snprintf((char*)buffer + 18,sizeof(buffer),"%c",id_part2);
	snprintf((char*)buffer + 19,sizeof(buffer),"%c",id_part1);

	memcpy(buffer + 20,"\x00\x00",2); // fragment offset (20 - 21)

	memcpy(buffer + 22,"\x80",1); // Time to live (22)

	memcpy(buffer + 23,"\x11",1); // UDP Identification - 17 (23)

	// IP header checksum - will be calculated later (24 - 25)
	memcpy(buffer + 24,"\x00\x00",2);

	memcpy(buffer + 26,source_ip,4); // IP source (26 - 29)

	memcpy(buffer + 30,destination_ip,4);  // IP destination (30 - 33)

	ip_checksum = ip_header_checksum(buffer +14,HEADER_SIZE);

	const uint8_t check_part2 = ip_checksum >> 8;
	const uint8_t check_part1 = ip_checksum;

	snprintf((char*)buffer + 24,sizeof(buffer),"%c",check_part2);
	snprintf((char*)buffer + 25,sizeof(buffer),"%c",check_part1);

	memcpy(buffer + 26,source_ip,4); // IP source (26 - 29)

	memcpy(buffer + 34,"\x27\x0f",2); // source port (34 - 35)

	memcpy(buffer + 36,"\x27\x0f",2); // destination port (36 - 37)

	// protocol length - data len + 8 (UDP header)
	memcpy(buffer + 38,"\x00",1);
	snprintf((char*)buffer + 39,sizeof(buffer),"%c",udp_length); // (38 - 39)

	memcpy(buffer + 40,"\x00\x00",2); // checksum (40 - 41) \xd3\xdf

	memcpy(buffer + 42,data,length); // data (42 - ...)

	ENET_TXQueue(42 + length);

	identification++;

	return ENET_IsTXFinish();

}
/* Základní ethernetová inicializace
 * - Nastavení všech pinů
 * - MAC konfigurace 100Mbps
 * - Nastavení MII frekvence hodin a PHY adresy
 * - Nastavení deskriptorů a povolení RX/TX
 * */
void enet_init(){
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0x1, 0, (IOCON_FUNC1 | IOCON_MODE_INACT));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0x1, 1, (IOCON_FUNC1 | IOCON_MODE_INACT));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0x1, 4, (IOCON_FUNC1 | IOCON_MODE_INACT));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0x1, 8, (IOCON_FUNC1 | IOCON_MODE_INACT));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0x1, 9, (IOCON_FUNC1 | IOCON_MODE_INACT));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0x1, 10, (IOCON_FUNC1 | IOCON_MODE_INACT));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0x1, 14, (IOCON_FUNC1 | IOCON_MODE_INACT));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0x1, 15, (IOCON_FUNC1 | IOCON_MODE_INACT));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0x1, 16, (IOCON_FUNC1 | IOCON_MODE_INACT));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0x1, 17, (IOCON_FUNC1 | IOCON_MODE_INACT));

	Chip_ENET_Init(LPC_ETHERNET, true);

	enet_setup();
}

