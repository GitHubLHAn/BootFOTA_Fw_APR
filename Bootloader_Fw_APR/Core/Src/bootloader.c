/*
 * bootloader.c 
 * Created on: 19-June-2025
 * Author: Le Huu An  (anlh55@viettel.com.vn)
 * Version: 1.0
 * MCU: STM32 H5
 */
 
	#include "bootloader.h"

	#include <string.h>
	
	#include "w5500_spi.h"
	#include "socket.h"
	#include "w5500.h"

/*
NOTE: 
	
*/
/**********************************************************************************************************************************/
	
	uint8_t Identify = 0;
				
	uint8_t RX_Buffer[NUM_BYTE_BUFFER];
	
	uint8_t MODE_FLASH = NOT_FLASH;
	
	uint32_t addr_start_flash;
	
	uint32_t addr_end_flash;
	
	uint8_t sector_start_flash;
	
	uint8_t number_sector_flash;
	
	uint32_t _start_time_reset = 0, _interval_time_reset = 0;
	
	AP_Infor_t AP_Infor_default ={
		.existData = 37,
		
		.mode_boot = BOOT_INTO_APPLICATION,
		.ver_app = 37, .type_board = 0x03, // APR board
		.day = 2, .month = 7, .year = 2025,
		.VTOR = ADDRESS_BOUNDARY_APP_START,
		
		.IP1 = 192, .IP2 = 168, .IP3 = 1, .IP4 = 101,
		.port = 1111,
		.des_IP1 = 192, .des_IP2 = 168, .des_IP3 = 1, .des_IP4 = 100,
		.des_port = 1111,
		.RF_frequency = 803,
		.channel = 0,
		
		.mode_filter_ID_RB = 1,
		.SP1 = 0
//*****add more initial parmas
		
	};
	
	AP_Infor_t AP_Infor_cache;
	
	uint8_t des_ip[4] = {0,0,0,0};		// ip destination
	uint16_t des_port = 0;					// port destination
	
	extern CRC_HandleTypeDef hcrc;

/*******************************************************************************
 @brief        Get the Flash sector corresponding to the given address.

 @param[in]    Address  - The memory address for which the Flash sector is to be determined.

 @return       FLASH_SECTOR_x constant based on the address provided.
               Returns 0xFFFFFFFF if the address is outside the valid Flash region.
*******************************************************************************/
	uint32_t GetSector(uint32_t Address)
	{
			if (Address < 0x08002000) return FLASH_SECTOR_0;
			else if (Address < 0x08004000) return FLASH_SECTOR_1;
			else if (Address < 0x08006000) return FLASH_SECTOR_2;
			else if (Address < 0x08008000) return FLASH_SECTOR_3;
			else if (Address < 0x0800A000) return FLASH_SECTOR_4;
			else if (Address < 0x0800C000) return FLASH_SECTOR_5;
			else if (Address < 0x0800E000) return FLASH_SECTOR_6;
			else if (Address < 0x08010000) return FLASH_SECTOR_7;
		
			else if (Address < 0x08012000) return FLASH_SECTOR_0;
			else if (Address < 0x08014000) return FLASH_SECTOR_1;
			else if (Address < 0x08016000) return FLASH_SECTOR_2;
			else if (Address < 0x08018000) return FLASH_SECTOR_3;
			else if (Address < 0x0801A000) return FLASH_SECTOR_4;
			else if (Address < 0x0801C000) return FLASH_SECTOR_5;
			else if (Address < 0x0801E000) return FLASH_SECTOR_6;
			else if (Address < 0x08020000) return FLASH_SECTOR_7;
			else return 0xFFFFFFFF; // invalid
	}

/*******************************************************************************
 @brief        Get the Flash memory bank corresponding to the given address.

 @param[in]    Address  - The memory address for which the Flash bank is to be determined.

 @return       FLASH_BANK_1 or FLASH_BANK_2 based on the address.
               Returns 0xFFFFFFFF if the address is outside the valid Flash memory space.	
*******************************************************************************/
	uint32_t GetBank(uint32_t Address)
	{
			if (Address < 0x08010000) return FLASH_BANK_1;
			else if (Address < 0x08020000) return FLASH_BANK_2;
			else return 0xFFFFFFFF; // invalid
	}

/*******************************************************************************
 @brief 			Erase the Flash memory.
 
 @param[in] 	@address	- Address of Pages/Sector to Erase.
							@NumPagesErase	-	number Pages/Setor to Erase.
								
 @return			Result Erasing HAL_OK (0x00) on SUCCESSFULFULFULFUL
														 HAL_ERROR (0x01) on failure.
*******************************************************************************/
	static uint8_t Flash_Erase(uint32_t address, uint8_t NumSectorsErase)
	{
		uint8_t result=0xEE;
		
		uint32_t sector = GetSector(address); 
		uint32_t bank = GetBank(address);
		
		FLASH_EraseInitTypeDef FlashEraseDefination;
		
		uint32_t FlashEraseFault = 0;
		HAL_FLASH_Unlock();
		
		FlashEraseDefination.TypeErase = FLASH_TYPEERASE_SECTORS;
		
		FlashEraseDefination.Banks = GetBank(address);
		FlashEraseDefination.NbSectors = NumSectorsErase;
		FlashEraseDefination.Sector = GetSector(address);
		
		result = HAL_FLASHEx_Erase(&FlashEraseDefination, &FlashEraseFault);
		HAL_FLASH_Lock();
		
		return result;
	}

/*******************************************************************************
 @brief 			Write an array on Flash memory.
 
 @param[in] 	@address	- address to write data.
							@arr			-	data array.
							@len			- length of data array.
								
 @return			HAL_OK (0x00) on SUCCESS. Otherwise, on failure.
*******************************************************************************/
uint8_t Flash_Write_Array(uint32_t address, uint8_t *arr, uint16_t len)
{
    uint8_t result;
    uint8_t buffer[16];
    uint16_t i;

    HAL_FLASH_Unlock();

    for (i = 0; i < len; i += 16)
    {
        uint16_t rem = len - i;
        memset(buffer, 0xFF, 16);
        memcpy(buffer, &arr[i], rem >= 16 ? 16 : rem);

        result = HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD,
																						address + i,
																							(uint32_t)buffer);
        if (result != HAL_OK) {
            HAL_FLASH_Lock();
            return 1;
        }

        while (__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY)) {}
    }

    HAL_FLASH_Lock();
    return 0;
}
	
	
/*******************************************************************************
 @brief 			Read an array from Flash memory.
 
 @param[in] 	@address	- address to read data.
							@arr			-	data array pointer.
							@len			- length of data array.
*******************************************************************************/
void Flash_Read_Array(uint32_t address, uint8_t *arr, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        arr[i] = *(__IO uint8_t *)(address + i);
    }
}

/*******************************************************************************
 @brief 			Calculating CRC8.
 
 @param[in] 	@data			- data pointer.
							@length		- length of data array.
	
@return			  @crc			- result of CRC8 calculating
*******************************************************************************/	
	uint8_t CRC8(uint8_t *data, uint8_t length) 
	{
    uint8_t crc = 0;
		
    for(uint8_t i = 0; i < length - 1; i++) 
		{
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) 
				{
            crc = (crc << 1) ^ (crc & 0x80 ? 0x07 : 0);
        }
    }
		return crc;
	}
	
/*******************************************************************************
 @brief       Read APR information from Flash memory into a structure.

 @param[in]   address   - Starting address in Flash where the AP information is stored.
 @param[out]  pDATA     - Pointer to a structure of type AP_Infor_t that will be
                          filled with the data read from Flash.
*******************************************************************************/
	void Flash_Read_APR_Infor(uint32_t address, AP_Infor_t *pDATA)
	{
		Flash_Read_Array(address, (uint8_t*)pDATA, sizeof(AP_Infor_t));
	}
		
/*******************************************************************************
 @brief       Write APR information to Flash memory.

 @param[in]   address   - Starting address in Flash where the data will be written.
 @param[in]   pDATA     - Structure of type AP_Infor_t containing the data to write.

 @return      Result of the Flash write operation:
              HAL_OK (0x00)    - Write successful.
              HAL_ERROR (0x01) - Write failed.
*******************************************************************************/
	uint8_t Flash_Write_APR_Infor(uint32_t address, AP_Infor_t pDATA)
	{
		return Flash_Write_Array(address, (uint8_t*)&pDATA, sizeof(AP_Infor_t));	
	}

/*******************************************************************************
 @brief       Bootloader decision function to determine whether to stay in 
              bootloader mode or jump to the application.

 @details     This function checks the `mode_boot` flag stored in Flash to determine
              whether to enter bootloader mode or jump to the main application.
*******************************************************************************/		
	typedef void (*pFunction)(void);
	
	uint8_t result_debug = 0;
	
	void Boodloader_Check(void)
	{		
		if(AP_Infor_cache.mode_boot == BOOT_INTO_BOOTLOADER)
		{
			OFF_LED_DEBUG(); HAL_Delay(50);
			ON_LED_DEBUG(); HAL_Delay(50);
			Identify = AP_Infor_cache.IP4;
		}
		else if(AP_Infor_cache.mode_boot == BOOT_INTO_APPLICATION){		// jump to application program
			
			uint8_t check_array[30];
			uint8_t app_exist = 0;
			
			Flash_Read_Array(ADDRESS_BOUNDARY_APP_START, check_array, 30);
			
			for(uint8_t ct=0; ct<30; ct++){
				if(check_array[ct] == 0xFF) app_exist++;
			}
			
			while(app_exist >= 25){		// Application does not exist
				OFF_LED_DEBUG(); HAL_Delay(100); ON_LED_DEBUG(); HAL_Delay(500);
				OFF_LED_DEBUG(); HAL_Delay(100); ON_LED_DEBUG(); HAL_Delay(500);
				OFF_LED_DEBUG(); HAL_Delay(100); ON_LED_DEBUG(); HAL_Delay(500);
				OFF_LED_DEBUG(); HAL_Delay(100); ON_LED_DEBUG(); HAL_Delay(1000);
				
				uint8_t result = 0;
				
				// erase storage data sector
				HAL_Delay(1);
				result += Flash_Erase(ADDRESS_DATA_STORAGE,1);
				HAL_Delay(10);
				
				AP_Infor_cache.mode_boot = BOOT_INTO_BOOTLOADER;
				AP_Infor_cache.crc_confirm = CRC8((uint8_t*)&AP_Infor_cache, sizeof(AP_Infor_t));
				
				result += Flash_Write_APR_Infor(ADDRESS_DATA_STORAGE, AP_Infor_cache);
				
				if(result == 0 || --app_exist == 0)
					NVIC_SystemReset();				
			}
			
			
			/*Start Jump to Application*/
			__disable_irq();
			
			HAL_DeInit();
			HAL_RCC_DeInit();
			SysTick->CTRL = SysTick->LOAD = SysTick->VAL = 0;

			for (int i = 0; i < 8; i++) {
					NVIC->ICER[i] = 0xFFFFFFFF;
					NVIC->ICPR[i] = 0xFFFFFFFF;
			}

			SCB->VTOR = 0x08008000U;
	
			uint32_t appStack = *(__IO uint32_t*) SCB->VTOR;
      __set_MSP(appStack);
			
      uint32_t reset_addr = *(__IO uint32_t*) (SCB->VTOR + 4U);
			reset_addr |= 0x1U;
			
			pFunction JumpToApp;
			JumpToApp = (pFunction) reset_addr;

			JumpToApp();
		}
		else{
			HAL_Delay(1);
			Flash_Erase(ADDRESS_DATA_STORAGE,1);
			
			NVIC_SystemReset();
		}
	}
	
/*******************************************************************************
 @brief 			Calculating CRC32, @refer CRC32 STM32 Hardware.
 
 @param[in] 	@start_addr			- start data address.
							@end_addr				- end data address.
	
 @return			  @crc32			- result of CRC32 calculating
*******************************************************************************/		
	uint32_t Calculate_CRC32(uint32_t start_addr, uint32_t end_addr) 
	{		
		uint32_t *flash_ptr = (uint32_t *)start_addr;
    uint32_t length = (end_addr - start_addr + 1) / 4;

    __HAL_CRC_DR_RESET(&hcrc); // Reset CRC
    return HAL_CRC_Accumulate(&hcrc, flash_ptr, length);
	}
	
/*******************************************************************************
 @brief 			Checking Reset Chip.
 
 @note				Soft Reset MCU if do not rx mess from PC too long time (1 minutes)
*******************************************************************************/		
	void Check_ResetChip(void) 
	{
    _interval_time_reset = HAL_GetTick() - _start_time_reset;
		if(_interval_time_reset >= 10000){
			//NVIC_SystemReset();
			_start_time_reset = HAL_GetTick();
		}
	}

/*******************************************************************************
 @brief 			Handling message received.
 @Note				Put this function on main while(1)
*******************************************************************************/
	void Handle_Mess_Rx(void)
	{
		uint8_t TX_Buffer[20];
		uint8_t result = FAILURE;
		
		/*Responding status------------------------------*/
		if(RX_Buffer[0] == 0xAB && RX_Buffer[1] == 0xCD &&
				RX_Buffer[2] == CMD_STATUS && RX_Buffer[11] == CRC8(RX_Buffer,12)){
			HAL_Delay(1);
			TOOGLEPIN_LED_DEBUG();
			_start_time_reset = HAL_GetTick();
			
			TX_Buffer[0] = Identify;
			TX_Buffer[1] = 15;
			TX_Buffer[2] = CMD_STATUS;
			TX_Buffer[3] = AP_Infor_cache.mode_boot;
			TX_Buffer[4] = AP_Infor_cache.ver_app;
			TX_Buffer[5] = AP_Infor_cache.day;
			TX_Buffer[6] = AP_Infor_cache.month;
			TX_Buffer[7] = (AP_Infor_cache.year >> 8) &0xFF;
			TX_Buffer[8] = AP_Infor_cache.year & 0xFF;
			TX_Buffer[9] =  (AP_Infor_cache.VTOR >> 24) & 0xFF;
			TX_Buffer[10] = (AP_Infor_cache.VTOR >> 16) & 0xFF;
			TX_Buffer[11] = (AP_Infor_cache.VTOR >> 8) & 0xFF;
			TX_Buffer[12] =  AP_Infor_cache.VTOR & 0xFF;
			TX_Buffer[13] = AP_Infor_cache.type_board;
			TX_Buffer[14] = CRC8(TX_Buffer, TX_Buffer[1]);
			
			sendto(1, (uint8_t *)TX_Buffer, TX_Buffer[1], des_ip, des_port);	
			
			memset(RX_Buffer, 0x00, NUM_BYTE_BUFFER);
			return;	
		}
		
		/*Check Identify header--------------------------*/
		uint8_t header = RX_Buffer[0];
		if(header != Identify){
			memset(RX_Buffer, 0x00, NUM_BYTE_BUFFER);
			return;
		}	
		uint8_t length_mess = RX_Buffer[1];
		
		/*Check CRC8-------------------------------------*/
		uint8_t check_crc8 = CRC8(RX_Buffer, length_mess);
		if(check_crc8 != RX_Buffer[length_mess-1]){
			memset(RX_Buffer, 0x00, NUM_BYTE_BUFFER);
			return;
		}
		
		/*Handling command-------------------------------*/
		uint8_t cmd = RX_Buffer[2];
		
		TOOGLEPIN_LED_DEBUG();				// received a correct message
		_start_time_reset = HAL_GetTick();
		
		/*Get new program information and Erase Flash*/
		if(cmd == CMD_START_FLASHING){
			result = SUCCESSFUL;
			uint32_t size_program_flash = 0;
			
			/*Get new information*/
			addr_start_flash = (RX_Buffer[3]<<24) | (RX_Buffer[4]<<16) | (RX_Buffer[5]<<8) | RX_Buffer[6];
			addr_end_flash = (RX_Buffer[7]<<24) | (RX_Buffer[8]<<16) | (RX_Buffer[9]<<8) | RX_Buffer[10];
				
			/*Check Boundary Application Condition*/
			if(addr_start_flash >= ADDRESS_BOUNDARY_APP_START && addr_end_flash <= ADDRESS_BOUNDARY_APP_END)
				result = SUCCESSFUL;
			else
				result = FAILURE;
			
			if(result == SUCCESSFUL)
			{
				size_program_flash = addr_end_flash - addr_start_flash;
				sector_start_flash = (addr_start_flash&0xFFFF) / 0x2000;
				number_sector_flash  = size_program_flash/0x2000;
			
				if(size_program_flash%0x2000 != 0) 
					number_sector_flash++;
				
				result = 0;	
				for(uint32_t addr_erase = addr_start_flash; addr_erase<addr_end_flash; addr_erase+= 0x2000)
				{
					result += Flash_Erase(addr_erase, 1);
					HAL_Delay(1);;
				}
	
				/*Erasing flash*/
				if(result == 0){
					result = SUCCESSFUL;
					MODE_FLASH = FLASHING;
				}
				else{
					result = FAILURE;
				}
			}
			/*Build response mess*/
				TX_Buffer[0] = Identify;
				TX_Buffer[1] = 6;
				TX_Buffer[2] = cmd;		
				TX_Buffer[3] = result;
				TX_Buffer[4] = number_sector_flash;
				TX_Buffer[5] = CRC8(TX_Buffer, TX_Buffer[1]);
					
				sendto(1, (uint8_t *)TX_Buffer, TX_Buffer[1], des_ip, des_port);	
				
				memset(RX_Buffer, 0x00, NUM_BYTE_BUFFER);
				return;
		}
			
		
		/*Flashing process-----------------------*/
		if(cmd == CMD_FLASHING){
			if(MODE_FLASH != FLASHING){
				return;
			}
			result = SUCCESSFUL;
			
			uint32_t address_flash = (RX_Buffer[3]<<24) | (RX_Buffer[4]<<16) | 
																(RX_Buffer[5]<<8) | RX_Buffer[6];			
			uint8_t size_flash = length_mess-8;
			
			if(address_flash >= addr_start_flash && 
						address_flash <= (addr_end_flash) &&
								address_flash+size_flash <= ADDRESS_BOUNDARY_APP_END)
				result = SUCCESSFUL;
			else
				result = FAILURE;
			
			if(result == SUCCESSFUL){
				uint8_t arr_flash[size_flash];
				uint8_t arr_flash_check[size_flash];
				
				for(uint8_t cnt=0; cnt<size_flash; cnt++)
					arr_flash[cnt] = RX_Buffer[cnt+7];

				Flash_Read_Array(address_flash, arr_flash_check, size_flash);
				HAL_Delay(1);
			
				if(memcmp(arr_flash,arr_flash_check,size_flash) == 0){
					result = SUCCESSFUL;			// data already exist
				}
				else{
					result = Flash_Write_Array(address_flash, arr_flash, size_flash);								
					if(result == HAL_OK)	result = SUCCESSFUL;
					else									result = FAILURE;
				}			
			}
					
			/*Build response mess*/	
			uint8_t TX_Buffer[6];
			TX_Buffer[0] = Identify;
			TX_Buffer[1] = 6;
			TX_Buffer[2] = CMD_FLASHING;		
			TX_Buffer[3] = result;
			TX_Buffer[4] = size_flash;				// quantity bytes flashed
			TX_Buffer[5] = CRC8(TX_Buffer, TX_Buffer[1]);
				
			sendto(1, (uint8_t *)TX_Buffer, TX_Buffer[1], des_ip, des_port);
			memset(RX_Buffer, 0x00, NUM_BYTE_BUFFER);
			return;
		}
		
		/*Verifying program flashed-----------------------------*/
		if(cmd == CMD_VERIFY_DATA){
			result = SUCCESSFUL;
			
			uint32_t crc32_getValue = (RX_Buffer[3]<<24) | (RX_Buffer[4]<<16) | 
																(RX_Buffer[5]<<8) | RX_Buffer[6];	
			/*Check CRC32 here*/
			
			uint32_t crc32_check = Calculate_CRC32(addr_start_flash, addr_end_flash);
			
			if(crc32_getValue == crc32_check)
				result = SUCCESSFUL;
			else
				result = FAILURE;
		
			uint8_t TX_Buffer[9];
			TX_Buffer[0] = Identify;
			TX_Buffer[1] = 9;
			TX_Buffer[2] = CMD_VERIFY_DATA;		
			TX_Buffer[3] = result;
			TX_Buffer[4] = (crc32_check>>24)&0xFF;
			TX_Buffer[5] = (crc32_check>>16)&0xFF;
			TX_Buffer[6] = (crc32_check>>8)&0xFF;
			TX_Buffer[7] = (crc32_check)&0xFF;
			TX_Buffer[8] = CRC8(TX_Buffer, TX_Buffer[1]);
				
			sendto(1, (uint8_t *)TX_Buffer, TX_Buffer[1], des_ip, des_port);
			
			memset(RX_Buffer, 0x00, NUM_BYTE_BUFFER);

			return;
		}
		
		/*Command Running Application----------------------------*/
		if(cmd == CMD_RUN_APP){			
			result = 0;
			
			result += Flash_Erase(ADDRESS_DATA_STORAGE, 1);		
			HAL_Delay(10);
			
			AP_Infor_t AP_Infor_new;
			memcpy(&AP_Infor_new, &AP_Infor_cache, sizeof(AP_Infor_t));
			
			AP_Infor_new.mode_boot = BOOT_INTO_APPLICATION;
			AP_Infor_new.ver_app = RX_Buffer[3];
			AP_Infor_new.day = RX_Buffer[4];
			AP_Infor_new.month = RX_Buffer[5];
			AP_Infor_new.year = (RX_Buffer[6]<<8) | RX_Buffer[7];
			AP_Infor_new.type_board = RX_Buffer[8];
			AP_Infor_new.crc_confirm = CRC8((uint8_t*)&AP_Infor_new, sizeof(AP_Infor_t));
					
			result += Flash_Write_APR_Infor(ADDRESS_DATA_STORAGE, AP_Infor_new);
			
			if(result == 0) result = SUCCESSFUL;
			else 						result = FAILURE;
			
			uint8_t TX_Buffer[5];
			TX_Buffer[0] = Identify;
			TX_Buffer[1] = 5;
			TX_Buffer[2] = CMD_RUN_APP;		
			TX_Buffer[3] = result;
			TX_Buffer[4] = CRC8(TX_Buffer, TX_Buffer[1]);
				
			sendto(1, (uint8_t *)TX_Buffer, TX_Buffer[1], des_ip, des_port);

			if(result == SUCCESSFUL)
				NVIC_SystemReset();
			
			memset(RX_Buffer, 0x00, NUM_BYTE_BUFFER);
			return;
		}
		
	}
	
/*******************************************************************************
 @brief       Initialize a local UDP server and open a listening port.

 @details     This function creates a UDP socket on socket number 1 using the port
              specified in `AP_Infor_cache.port`. It sets the socket to blocking I/O mode.

 @return      SUCCESSFUL (0x00) - If the socket is successfully opened and configured.  
              FAILURE   (0x01)  - If socket creation fails.
*******************************************************************************/
	uint8_t Make_Local_Server(void)
	{
		/***Make a Server and open a listening port***/	
		if(socket(1, Sn_MR_UDP, AP_Infor_cache.port, 0) != 1){
			return FAILURE;
		}			
		else{			
			uint8_t socket_io_mode = SOCK_IO_BLOCK;
			ctlsocket(1,CS_SET_IOMODE,&socket_io_mode);
			
			return SUCCESSFUL;
		}
	}

/*******************************************************************************
 @brief       Load and verify application parameters from Flash memory.

 @details     This function reads APR information from Flash into 
              `AP_Infor_cache` and verifies it using a CRC check.
*******************************************************************************/
	void Get_AP_Informations(void)
	{		
		uint8_t result = 0;
		
		Flash_Read_APR_Infor(ADDRESS_DATA_STORAGE, &AP_Infor_cache);
		
		uint8_t check_confirm = CRC8((uint8_t*)&AP_Infor_cache, sizeof(AP_Infor_t));
			
		if(AP_Infor_cache.existData != AP_Infor_default.existData && 
											AP_Infor_cache.crc_confirm != check_confirm)
		{
			// erase storage data sector
			HAL_Delay(1);
			result += Flash_Erase(ADDRESS_DATA_STORAGE,1);
			HAL_Delay(10);
			
			//	write default params			
			AP_Infor_default.crc_confirm = CRC8((uint8_t*)&AP_Infor_default, sizeof(AP_Infor_t));
			result += Flash_Write_APR_Infor(ADDRESS_DATA_STORAGE, AP_Infor_default);

			uint8_t lan = 3;	
			while(lan>0 && result==0){
				ON_LED_DEBUG();HAL_Delay(50);OFF_LED_DEBUG();HAL_Delay(150);
				ON_LED_DEBUG();HAL_Delay(50);OFF_LED_DEBUG();HAL_Delay(500);
				lan--;
			}
			
			NVIC_SystemReset();
		}
	}

/*******************************************************************************
 @brief       Initialize and configure the W5500 Ethernet module.
*******************************************************************************/
	void Setup_Ethernet(void)
	{
		OFF_LED_DEBUG();
		
		uint8_t result_config = 0;	
		
		/*Setup_Ethernet*/
		w5500_Init();	
		result_config = w5500_Config();
		
		uint8_t lan = 20;
		while(result_config == 0){
			ON_LED_DEBUG();HAL_Delay(10);OFF_LED_DEBUG();HAL_Delay(300);
			ON_LED_DEBUG();HAL_Delay(10);OFF_LED_DEBUG();HAL_Delay(300);
			ON_LED_DEBUG();HAL_Delay(10);OFF_LED_DEBUG();HAL_Delay(700);
			if(--lan == 0)	NVIC_SystemReset();
		}

		/*Make a Server and open a listening port*/	
		result_config = Make_Local_Server();
		
		lan = 20;
		while(result_config == FAILURE){
			ON_LED_DEBUG();HAL_Delay(10);OFF_LED_DEBUG();HAL_Delay(300);
			ON_LED_DEBUG();HAL_Delay(10);OFF_LED_DEBUG();HAL_Delay(300);
			ON_LED_DEBUG();HAL_Delay(10);OFF_LED_DEBUG();HAL_Delay(300);
			ON_LED_DEBUG();HAL_Delay(10);OFF_LED_DEBUG();HAL_Delay(700);
			if(--lan == 0)	NVIC_SystemReset();	
		}

		memset(RX_Buffer, 0x00, NUM_BYTE_BUFFER);
		_start_time_reset = HAL_GetTick();
		
		ON_LED_DEBUG();
	}


/*******************************************************************************
 @brief       Read and process incoming UDP packets from Ethernet (W5500).

 @note        - This function only handles UDP (socket status 0x22).
              - It assumes the last byte of the packet is a CRC8 checksum.
              - Socket 1 is used for all operations.

*******************************************************************************/
	void Read_ETHERNET(void)
	{
		uint8_t sr = 0x00;
		
		sr = getSn_SR(1);		//16us
		if(sr == 0x00){		// do not have any client
			return;
		}
	
		if(sr == 0x22)		// 0x17 if TCP, 0x22 if UDP
		{				
			/*Receiving request/cmd from PC and transmit to robot*/
			uint8_t rx_size = getSn_RX_RSR(1);
						
			if(rx_size != 0 && rx_size>=8)
			{
				rx_size = rx_size - 8;
								
				/*Get the data from W5500 FIFO*/
				recvfrom(1, (uint8_t *)RX_Buffer, rx_size, des_ip, &des_port);			
				
				/*Check CRC8*/
				uint8_t check_byte = CRC8(RX_Buffer, rx_size);					
				
				if(check_byte == RX_Buffer[rx_size-1])	//mess ok
				{						
					Handle_Mess_Rx();
				}
			}
		}
	}
	
/**********************************************************************************************************************************/