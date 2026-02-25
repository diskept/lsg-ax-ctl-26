
#include "eeprom.h"
 
uint16_t E2P_Create(void) 
{
	// unlock flash
	HAL_FLASH_Unlock();

	return E2P_DENSITY_BYTES;
}

HAL_StatusTypeDef E2P_Erase (void)
{
	static FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t PageError = 0;

	// unlock flash
	HAL_FLASH_Unlock();

	/* Fill EraseInit structure*/
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.PageAddress = E2P_PAGE_BASE_ADDRESS;
	EraseInitStruct.NbPages = E2P_DENSITY_PAGES;

	if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK) {
		if(PageError != 0xFFFFFFFF)
			return HAL_ERROR;
		else
			return HAL_OK;
	}
	return HAL_BUSY;
}

HAL_StatusTypeDef E2P_Write_Byte (uint16_t Address, uint16_t DataByte)
{
	uint32_t	page;
	
	HAL_StatusTypeDef status = HAL_ERROR;

	// exit if desired address is above the limit (e.G. under 2048 Bytes for 4 pages)
	if (Address > E2P_DENSITY_BYTES) {
		return 0;
	}

	// calculate which page is affected (Pagenum1/Pagenum2...PagenumN)
	page = (E2P_PAGE_BASE_ADDRESS + E2P_ADDR_OFFSET(Address)) & 0x00000FFF;

	if (page % E2P_PAGE_SIZE)	page = page + E2P_PAGE_SIZE;
	page = (page / E2P_PAGE_SIZE) - 1;

	// if current data is 0xFF, the byte is empty, just overwrite with the new one
	if ((*(uint16_t*)(E2P_PAGE_BASE_ADDRESS + E2P_ADDR_OFFSET(Address))) == E2P_EMPTY_WORD)
		status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, E2P_PAGE_BASE_ADDRESS + E2P_ADDR_OFFSET(Address), DataByte);

	return status;
}

uint16_t E2P_Read_Byte (uint16_t Address) 
{
	uint16_t DataByte = 0xFFFF;

	// Get Byte from specified address
	DataByte = (*(uint16_t*)(E2P_PAGE_BASE_ADDRESS + E2P_ADDR_OFFSET(Address)));

	return DataByte;
}

void E2P_Destroy(void)
{
	// lock flash
	HAL_FLASH_Lock();
}

void E2P_Default_Config_Save(CONFIG_DATA* Save)
{
	E2P_Write_Byte(DEFAULT_CONFIG_ADDR, Save->nNode485Baud);
	E2P_Write_Byte(DEFAULT_CONFIG_ADDR+1, Save->nAct485Baud);
}

void E2P_Default_Config_Load(CONFIG_DATA* Load)
{
	Load->nNode485Baud = E2P_Read_Byte(DEFAULT_CONFIG_ADDR);
	Load->nAct485Baud = E2P_Read_Byte(DEFAULT_CONFIG_ADDR+1);
}

void E2P_Install_Config_Save(CONFIG_DATA* Save)
{
	E2P_Write_Byte(INSTALL_CONFIG_ADDR, Save->nNode485Baud);
	E2P_Write_Byte(INSTALL_CONFIG_ADDR+1, Save->nAct485Baud);
}

void E2P_Install_Config_Load(CONFIG_DATA* Load)
{
	Load->nNode485Baud = E2P_Read_Byte(INSTALL_CONFIG_ADDR);
	Load->nAct485Baud = E2P_Read_Byte(INSTALL_CONFIG_ADDR+1);
}


