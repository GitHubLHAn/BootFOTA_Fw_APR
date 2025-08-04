/*
 * bootloader.c 
 * Created on: 19-June-2025
 * Author: Le Huu An  (anlh55@viettel.com.vn)
 * Version: 1.0
 */

#ifndef BOOTLOADER_H_
#define BOOTLOADER_H_

#include "main.h"



/*DEFINE*/

	//#define ADDRESS_Infor ((uint32_t)0x0801A000U)				// sector 5 bank 2
	#define ADDRESS_DATA_STORAGE ((uint32_t)0x0801E000U)								// sector 7 bank 2

	#define ADDRESS_BOUNDARY_APP_START ((uint32_t)0x08008000U)				// start sector 3 bank 1
	#define ADDRESS_BOUNDARY_APP_END 	 ((uint32_t)0x0801BFFFU)				// end sector 5 bank 2
	
	#define BOOT_INTO_BOOTLOADER  0x11
	#define BOOT_INTO_APPLICATION 0x22
	
	#define ON_LED_DEBUG(	)			HAL_GPIO_WritePin(LED_DEBUG_GPIO_Port, LED_DEBUG_Pin, GPIO_PIN_RESET);
	#define OFF_LED_DEBUG(	)		HAL_GPIO_WritePin(LED_DEBUG_GPIO_Port, LED_DEBUG_Pin, GPIO_PIN_SET);
	
	#define TOOGLEPIN_LED_DEBUG(	)	HAL_GPIO_TogglePin(LED_DEBUG_GPIO_Port, LED_DEBUG_Pin);
	
	#define NUM_BYTE_BUFFER 50
	
	#define NOT_FLASH	0
	#define FLASHING	1
	
	#define SUCCESSFUL			0x59
	#define FAILURE				0x4E	
	
	#define CMD_STATUS					0xB4
	#define CMD_RUN_BOOTLOADER	0xA1
	
	#define CMD_START_FLASHING	0xA2
	#define CMD_FLASHING				0xA3
	#define CMD_VERIFY_DATA			0xA4
	#define CMD_RUN_APP					0xA5

/************************************************************************************/
/*DECLARE STRUCT*/
	
	#pragma pack(1)
	typedef struct{
		// check for initiating default params
		uint8_t existData;

		// check bootloader
		volatile uint8_t mode_boot;
		uint8_t ver_app;	
		uint8_t day, month;
		uint16_t year;
		uint8_t type_board;
		volatile uint32_t VTOR;
		
		// information params
		uint8_t IP1, IP2, IP3, IP4;
		uint16_t port;	
		uint8_t des_IP1, des_IP2, des_IP3, des_IP4;
		uint16_t des_port;	
		uint16_t RF_frequency;
		uint8_t channel;
		
		uint8_t mode_filter_ID_RB;
		uint8_t SP1;
		uint8_t SP2;
		uint8_t SP3;
		uint8_t SP4;
		uint8_t SP5;
		uint8_t SP6;
		uint8_t SP7;
		uint8_t SP8;
		uint8_t SP9;
		uint8_t SP10;
		
		// check end of struct
		volatile uint8_t crc_confirm;
	}AP_Infor_t;
	#pragma pack()
	
			
	extern AP_Infor_t AP_Infor_cache;
	
	extern uint8_t des_ip[4];		// ip destination
	extern uint16_t des_port;					// port destination
	
	void Check_ResetChip(void);
	
	void Boodloader_Check(void);
	
	void Setup_Ethernet(void);
	
	void Read_ETHERNET(void)	;

	void Get_AP_Informations(void);
	
/************************************************************************************/
#endif /* BOOTLOADER_H */

