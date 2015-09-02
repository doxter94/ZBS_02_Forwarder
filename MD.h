/** @file MD.h*/
#ifndef MD_H_
#define MD_H_
#include <stdint.h>
#include "chip.h"
#include <LPC178x.h>

/* definice maker */
#define TRUE 1
#define FALSE 0
#define MEMORY_SIZE 1048576 /* 1 048 576 bytů */
#define PAGE_SIZE 256 /* 256 bytů */
#define BLOCK_SIZE 65536 /* 64KB */
#define SECTOR_SIZE 4096 /* 4KB */
#define PAGE_NUMBER 4096 /* 4096 stránek po 256 bytech = 1 048 576 bytů */
#define SECTOR_NUMBER 256 /* 256 sektorů po 4096 bytech = 1 048 576 bytů */
#define BLOCK_NUMBER 16 /* 16 bloků po 65536 bytech = 1 048 576 bytů */
#define DEFAULT_SSP LPC_SSP1
#define CS_port 0
#define CS_pin 6
#define SET_SPEED 1000000 /* 100kHz */

/* makra s kódem příkazů */
#define WRITE_ENABLE (uint8_t) 0x06
#define WRITE_DISABLE (uint8_t) 0x04
#define BLOCK_ERASE (uint8_t) 0xD8
#define SECTOR_ERASE (uint8_t) 0x20
#define CHIP_ERASE (uint8_t) 0xC7
#define READ_STATUS (uint8_t) 0x05
#define WRITE_STATUS (uint8_t) 0x01
#define READ_DATA (uint8_t) 0x03
#define PROGRAM_PAGE (uint8_t) 0x02
#define CHIP_ID (uint8_t) 0x9F

/* makra s ID zařízení */
#define MANOFACTURE (uint8_t) 0x01
#define MEMORY_TYPE (uint16_t) 0x4014


/* deklarace funkcí */


/*
 	 * funkce identifikuje paměťový chip a vrátí hodnotu TRUE v případě že se jedná o
 	 * s25fl208, nebo FALSE pokud je to jiný chip
*/
uint8_t identify_memory_chip();

/*
 	 * inicializuje rozhraní SPI a zavolá funkci identify_memory_chip()
 	 * pokud proběhne vše vpořádku vrátí hodnotu TRUE, v opačném případě hodnotu FALSE
*/
uint8_t init();

/*
 	 * funkce vyšle příkaz write enable, díky kterému je povolen zápis do paměti, funkce vrací TRUE nebo FALSE
*/
uint8_t write_enable();

/*
 	 * funkce vyšle příkaz write disable, díky kterému je zápis do paměti zakázán, funkce vrací TRUE nebo FALSE
*/
uint8_t write_disable();


/*
 	 * funkce testuje bit status registru WIP (Write In Progress), který signalizuje, zda je paměť připravena přijmout a vykonat daný příkaz,
 	 * funkce vrací hodnoty TRUE nebo FALSE
*/
uint8_t is_bussy();

/*
 	 * funkce vymaže sektor paměti v daném bloku, který je dán adresou (indexem) sektoru a adresou (indexem) bloku,
 	 * funkce vrátí hodnotu TRUE nebo FALSE
 	 * parametr sector_index - číslo (index) sektoru v intervalu 0 - 15
*/
uint8_t sector_erase(uint8_t sector_index);

/*
 	 * funkce vymaže daný blok paměti, který je dán adresou (indexem)
 	 * parametr block_index - udává adresu daného bloku v daném sektoru, interval 0 - 15
*/
uint8_t block_erase(uint8_t block_index);

/*
 	 * Vymaže celou flashku
*/
uint8_t flash_erase();

/*
 	 	 * funkce umožňuje naprogramovat (zapsat data) danou stránku, která je dána její adresou (indexem), funkce vrací TRUE nebo FALSE
 	 	 * parametr page_index - adresa (index) stránky, interval 0 - 4095
 	 	 * parametr data - ukazatel na pole 8-bitových dat, které se mají zapsat do paměti (do dané stránky), max 256 prvků pole
 	 	 * param length - delka pole dat v bajtech
*/
uint8_t program_page(uint16_t page_index, uint8_t* data,uint16_t length);

/*
 	 * funkce čte data z dané stránky, která je dána její adresou (indexem)
 	 * parametr page_index - adresa (index) stránky, interval 0 - 4095
*/
uint8_t read_page(uint16_t page_index,uint8_t *input_data);

/*
 	 * funkce čte hodnoty ve status registru paměti, funkce vrací 8-bitovou hodnotu, která reprezentuje stav registru.
*/
uint8_t read_status_registr();

/*
 	 * funkce umožňuje modifikovat hodnoty status registru paměti
 	 * parametry a,b,c,d,e značí bit lokace ve status registru (a = SPR,b = BP3,c = BP2,d = BP1, e = BP0)
*/
uint8_t modify_status_registr(uint8_t a,uint8_t b,uint8_t c,uint8_t d,uint8_t e);

#endif /* MD_H_ */
