
#ifndef __EEPROM_H
#define __EEPROM_H

#include "define.h"
#include "struct.h" 

// CAN BE CHANGED
#define E2P_DENSITY_PAGES				1			// how many pages are used 
#define E2P_PAGE_SIZE					2048	    // can be 1k or 2k check manual for used device
#define E2P_PAGE_BASE_ADDRESS			0x0801F800	// choose location for the first EEPROMPage address on the top of flash

// DONT CHANGE
#define E2P_DENSITY_BYTES				((E2P_PAGE_SIZE / 2) * E2P_DENSITY_PAGES - 1)
#define E2P_LAST_PAGE_ADDRESS 			(E2P_PAGE_BASE_ADDRESS + (E2P_PAGE_SIZE * E2P_DENSITY_PAGES))
#define E2P_EMPTY_WORD					((uint16_t)0xFFFF)
#define E2P_ADDR_OFFSET(Address)		(Address * 2) // 1Byte per Word will be saved to preserve Flash

uint16_t	E2P_Create(void) ;
HAL_StatusTypeDef E2P_Erase (void);
HAL_StatusTypeDef E2P_Write_Byte (uint16_t Address, uint16_t DataByte);
uint16_t	E2P_Read_Byte (uint16_t Address) ;
void		E2P_Destroy(void) ;

void E2P_Default_Config_Save(CONFIG_DATA* Save);
void E2P_Default_Config_Load(CONFIG_DATA* Load);
void E2P_Install_Config_Save(CONFIG_DATA* Save);
void E2P_Install_Config_Load(CONFIG_DATA* Load);

#endif

