/** @file MD.c*/
#include "MD.h"

/**
 * Set ISP CS pin to log 1 high
 *
 * @return nothing
 */
STATIC INLINE void CS_deselect()
{
	/* nastavení logické úrovně high na pinu CS rozhraní ISP */
	Chip_GPIO_SetPinOutHigh(LPC_GPIO, CS_port, CS_pin);

}

/*
 	 * funkce nastaví pin CS u ISP rozhraní (paměťového čipu) na úroveň logická 0 (low), tím je obvod uveden do aktivního
 	 * stavu, pokud není hodnota CS na úrovni low, ignoruje posílané příkazy - před každým příkazem je třeba volat tuto funkci
*/
STATIC INLINE void CS_select()
{
	/* nastavení logické úrovně low na pinu CS rozhraní ISP */
	Chip_GPIO_SetPinOutLow(LPC_GPIO, CS_port, CS_pin);

}

uint8_t identify_memory_chip()
{

	/* kontrola zda není paměť zaneprázdněná (opakování dokud není volná) */
	uint8_t bussy;
	do
	{
		 bussy= is_bussy();
	}
	while(bussy != FALSE);

	uint8_t id[3];
	uint8_t output_data = CHIP_ID;

	/* uvedení čipu do aktivního stavu */
	CS_select();

	/* vyslání příkazu pro identifikaci paměťového chipu */
	if(Chip_SSP_WriteFrames_Blocking(LPC_SSP1, &output_data,(uint32_t) sizeof(output_data)) != sizeof(output_data))
	{
		CS_deselect();
		return FALSE;
	}

	/* přijezí identifikačních dat */
	if(Chip_SSP_ReadFrames_Blocking(LPC_SSP1, id, (uint32_t) sizeof(id))!= sizeof(id))
	{
		CS_deselect();
		return FALSE;
	}

	/* identifikace zařízení */

	if(id[0] != MANOFACTURE)
	{
		CS_deselect();
		return FALSE;
	}

	if(id[1] != (uint8_t)(MEMORY_TYPE & 0xff00) && id[2] != (uint8_t)(MEMORY_TYPE & 0x00ff))
	{
		CS_deselect();
		return FALSE;
	}

	/* uvedení čipu do neaktivního stavu */
	CS_deselect();

	return TRUE;
}

uint8_t init()
{
	/* inicializace SPI rozhraní (nelze identifikovat čip dokud není inicializované rozhraní) */
	Chip_IOCON_Init(LPC_IOCON);
	Chip_GPIO_Init(LPC_GPIO);

    /* MISO, MOSI, SS and SCK pin-select */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 6, (IOCON_FUNC0 | IOCON_MODE_PULLUP)); // SS (Slave Select)
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 6);
	CS_deselect();
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 7, (IOCON_FUNC2 | IOCON_MODE_INACT | IOCON_HYS_EN | IOCON_DIGMODE_EN)); // CLK (Serial Clock)
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 8, (IOCON_FUNC2 | IOCON_MODE_INACT | IOCON_HYS_EN | IOCON_DIGMODE_EN)); // MISO (Master In Slave Out)
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 9, (IOCON_FUNC2 | IOCON_MODE_INACT | IOCON_HYS_EN | IOCON_DIGMODE_EN)); // MOSI (Master Out Slave In)

	/* Enable clock, set mode master, set format and set bit-rate */
	Chip_SSP_Init(LPC_SSP1);
	Chip_SSP_SetBitRate(LPC_SSP1, SET_SPEED);

	Chip_SSP_Enable(LPC_SSP1);

	/* identifikace paměťového čipu a navrácení hodnoty TRUE nebo FALSE */

	return identify_memory_chip();
}

uint8_t write_enable()
{
	/* uvedení čipu do aktivního stavu */
	CS_select();

	/* odeslání příkazu */

	uint8_t output_data = WRITE_ENABLE;

	if(Chip_SSP_WriteFrames_Blocking(LPC_SSP1 , &output_data, (uint32_t) sizeof(output_data))!=sizeof(output_data))
	{
		CS_deselect();
		return FALSE;
	}

	/* uvedení čipu do neaktivního stavu */
	CS_deselect();
	return TRUE;
}

uint8_t write_disable()
{
	/* uvedení čipu do aktivního stavu */
	CS_select();

	/* vyslání příkazu */

	uint8_t output_data = WRITE_DISABLE;
	if(Chip_SSP_WriteFrames_Blocking(LPC_SSP1 , &output_data, (uint32_t) sizeof(output_data))!=sizeof(output_data))
	{
		CS_deselect();
		return FALSE;
	}

	/* uvedení čipu do neaktivního stavu */
	CS_deselect();

	return TRUE;
}

uint8_t is_bussy()
{
	uint8_t status =  read_status_registr();
	uint8_t mask = 0b00000001;
	status &= mask;

	return status;
}

uint8_t sector_erase(uint8_t sector_index){

	/* makro  SECTOR_NUMBER udává počet sektorů, ale jsou indexovány od 0, proto -1*/
	if(sector_index > (SECTOR_NUMBER-1))
	{
		return FALSE;
	}

	/* kontrola zda není paměť zaneprázdněná */
	uint8_t bussy;
	do
	{
		 bussy = is_bussy();
	}
	while(bussy != FALSE);

	/* povolení pro zápis */
	write_enable();

	/* sestavení příkazu */
	uint32_t sector_addr = (sector_index * 4096);
	uint8_t output_data[4];
	output_data[0] = SECTOR_ERASE;
	int i;
    for (i=1; i<4 ;++i)
    	output_data[i] = ((uint8_t*)&sector_addr)[3-i];

	/* aktivace paměťového čipu */
	CS_select();

	/* vyslání kódu pro vymázání daného sektoru */
	if(Chip_SSP_WriteFrames_Blocking(LPC_SSP1,output_data, (uint32_t) sizeof(output_data)) != sizeof(output_data))
	{
		CS_deselect();
		return FALSE;
	}

	/* deaktivace paměťového čípu */
	CS_deselect();

	return TRUE;
}

uint8_t block_erase(uint8_t block_index)
{
	/* makro  BLOCK_NUMBER udává počet bloků, ale jsou indexovány od 0, proto -1*/
	if(block_index > (BLOCK_NUMBER - 1))
	{
		return FALSE;
	}

	/* sestavení příkazu a adresy */
	uint8_t output_data[4];	/* 1 bajt kód příkazu a 3 bajty adresa bloku */
	output_data[0] = BLOCK_ERASE;	/* příkaz */
	output_data[1] = block_index;	/* index bloku */
	output_data[2] = 0; /* horních 8 bitů adresy, zarovnání adresy */
	output_data[3] = 0; /* dolních 8 bitů adresy, zarovnání adresy */

	/* kontrola zda není paměť zaneprázdněná */
	uint8_t bussy;
	do
	{
		 bussy= is_bussy();
	}
	while(bussy != FALSE);

	/* povolení zápisu */
	write_enable();

	CS_select();
	/* vyslání příkazu pro vymazání daného bloku */

	if(Chip_SSP_WriteFrames_Blocking(LPC_SSP1, output_data, (uint32_t)sizeof(output_data))!=sizeof(output_data))
	{
		CS_deselect();
		return FALSE;
	}

	/* uvedení čipu do neaktivního stavu */
	CS_deselect();

	return TRUE;
}

uint8_t flash_erase(){

	/* kontrola zda není paměť zaneprázdněná */
	uint8_t bussy;
	do
	{
		 bussy= is_bussy();
	}
	while(bussy != FALSE);

	/* povolení zápisu */
	write_enable();

	CS_select();

	uint8_t output_data[1];
	output_data[0] = CHIP_ERASE;

	if(Chip_SSP_WriteFrames_Blocking(LPC_SSP1, output_data, (uint32_t)sizeof(output_data))!=sizeof(output_data))
	{
		CS_deselect();
		return FALSE;
	}

	/* uvedení čipu do neaktivního stavu */
	CS_deselect();

	return TRUE;
}


uint8_t program_page(uint16_t page_index, uint8_t* data,uint16_t length)
{
	/* validace vstupních dat */

	if(page_index > (PAGE_NUMBER - 1))
	{
		return FALSE;
	}

	if(sizeof(data) > PAGE_SIZE)
	{
		return FALSE;
	}


	/* sestavení příkazu */
	uint32_t page_addr = (page_index * 256);
	uint8_t output_data[4];	/* 1 bajt kód příkazu a 3 bajty adresa stránky */
	output_data[0] = PROGRAM_PAGE; /* kód příkazu */
	int i;
    for (i=1; i<4 ;++i)
    	output_data[i] = ((uint8_t*)&page_addr)[3-i];


	/* kontrola zda není paměť zaneprázdněná */
	uint8_t bussy;
	do
	{
		 bussy= is_bussy();
	}
	while(bussy != FALSE);


	/* povolení zápisu do paměti */
	write_enable();

	/* uvedení čipu do aktivního stavu */
	CS_select();

	/* odeslání příkazu */
	if(Chip_SSP_WriteFrames_Blocking(LPC_SSP1 , output_data, 4) != 4)
	{
		CS_deselect();
		return FALSE;
	}
	if(Chip_SSP_WriteFrames_Blocking(LPC_SSP1 , data, length) != length)
	{
		CS_deselect();
		return FALSE;
	}


	/* uvedení čipu do neaktivního stavu */
	CS_deselect();

	return TRUE;
}

uint8_t read_page(uint16_t page_index, uint8_t *input_data)
{
	/* validace vstupních dat */
	if(page_index > (PAGE_NUMBER - 1))
	{
		return FALSE;
	}

	/* sestavení příkazu */
	uint32_t page_addr = (page_index * 256);
	uint8_t output_data[4];	/* 1 bajt kód příkazu a 3 bajty adresa stránky */
	output_data[0] = READ_DATA;
	int i;
    for (i=1; i<4 ;++i)
    	output_data[i] = ((uint8_t*)&page_addr)[3-i];


	/* kontrola zda není paměť zaneprázdněná */
	uint8_t bussy;
	do
	{
		 bussy= is_bussy();
	}
	while(bussy != FALSE);

	/* uvedení čipu do aktivního stavu */
	CS_select();

	/* odeslání příkazu s parametry pro čtení dat */
	if(Chip_SSP_WriteFrames_Blocking(LPC_SSP1 , output_data, (uint32_t)sizeof(output_data)) != sizeof(output_data))
	{
		CS_deselect();
		return FALSE;
	}

	/* čtení dat */
	if( Chip_SSP_ReadFrames_Blocking(LPC_SSP1, input_data, PAGE_SIZE)!=PAGE_SIZE)
	{
		CS_deselect();
		return FALSE;
	}

	/* uvedení čipu do neaktivního stavu */
	CS_deselect();

	return TRUE;
}

uint8_t read_status_registr()
{
	uint8_t status = 0;
	/* uvedení čipu do aktivního stavu */
	CS_select();

	/* odeslání příkazu pro čtení status registru */
	uint8_t output_data = READ_STATUS;
	if(Chip_SSP_WriteFrames_Blocking(LPC_SSP1, &output_data, (uint32_t)sizeof(output_data)) != sizeof(output_data))
	{
		CS_deselect();
		return FALSE;
	}

	/* čtení dat ze status registru */
	if( Chip_SSP_ReadFrames_Blocking(LPC_SSP1, &status, sizeof(status)) != sizeof(status))
	{
		CS_deselect();
		return FALSE;
	}

	/* uvedení čipu do neaktivního stavu */
	CS_deselect();
	return status;
}

uint8_t modify_status_registr(uint8_t a,uint8_t b,uint8_t c,uint8_t d,uint8_t e)
{
	/* validace vstupních dat */
	if((a > 1 || a < 0) || (b > 1 || b < 0) || (c > 1 || c < 0) || (d > 1 || d < 0) || (e > 1 || e < 0))
	{
		return FALSE;
	}

	/* pole s příkazem a hodnotami */
	uint8_t output_data[2];

	/* nastavení příkazu a dat */
	output_data[0] = WRITE_STATUS;
	uint8_t pom;

	pom = ((a*32) + (b*8) + (c*4) + (d*2) + e)<<2;

	output_data[1] = pom;

	/* kontrola zda není paměť zaneprázdněná */
	uint8_t bussy;
	do
	{
		 bussy= is_bussy();
	}
	while(bussy != FALSE);

	/* povolení zápisu do paměti (status registru) */
	write_enable();

	/* uvedení čipu do aktivního stavu */
	CS_select();

	/* odeslání příkazu pro modifikaci status regitru */
	if(Chip_SSP_WriteFrames_Blocking(LPC_SSP1 , output_data, (uint32_t) sizeof(output_data)) != sizeof(output_data))
	{
		CS_deselect();
		return FALSE;
	}

	/* uvedení čipu do neaktivního stavu */
	CS_deselect();

	return TRUE;
}
