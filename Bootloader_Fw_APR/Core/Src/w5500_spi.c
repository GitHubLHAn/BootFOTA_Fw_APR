/*
	@NOTE: 
	
*/


	#include "w5500_spi.h"
	#include "wizchip_conf.h"

	#include "socket.h"
	#include "w5500.h"

	#include "bootloader.h"


///**********************************************************************************************************************************/

/*Extern*/
extern SPI_HandleTypeDef hspi2;


/**********************************************************************************************************************************/
/*Declare variable*/
	wiz_NetInfo gwIZNETINFO = {
		.mac 	= {0, 0, 0, 0, 0, 0},	
		.ip 	= {0, 0, 0, 0},					
		.sn 	= {255, 255, 225 , 0},					
		.gw 	= {192, 168, 1, 1},
		.dns  = {8, 8, 8, 8},
		.dhcp = NETINFO_STATIC };	

	
	uint8_t rx_tx_buff_sizes[] = {2,2,2,2,2,2,2,2};
	uint8_t memsize[2][8] = {{2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2} };
		

/**********************************************************************************************************************************/
/*FUNCTION*/
/*____________________________________________________________________________________*/
	static void PHYStatusCheck(void){
		uint8_t tmp;

		ctlwizchip(CW_GET_PHYCONF, (void*) &tmp);
		
		if(tmp == PHY_LINK_OFF)
		{		
			HAL_Delay(1500);
		}
			
		//my_printf("Good! Cable got connected!");
	}
/*____________________________________________________________________________________*/
	static void PrintPHYConf(void){
		wiz_PhyConf phyconf;
		
		ctlwizchip(CW_GET_PHYCONF, (void*)&phyconf);
		if(phyconf.by == PHY_CONFBY_HW)
		{
			//my_printf("\n\rPHY COnfigured by Hardware Pins");
		}
		else
		{
			//my_printf("\n\rPHY COnfigured by Registers");
		}
		if(phyconf.mode == PHY_MODE_AUTONEGO)
		{
			//my_printf("\n\rAutonegotiation Enable");
		}
		else
		{
			//my_printf("\n\rAutonegotiation NOT Enable");
		}
		if(phyconf.duplex == PHY_DUPLEX_FULL)
		{
			//my_printf("\n\rDuplex Mode: Full");
		}
		else
		{
			//my_printf("\n\rDuplex Mode: Hafl");
		}
		if(phyconf.speed == PHY_SPEED_10)
		{
			//my_printf("\n\rSpeed: 10Mbps");
		}
		else
		{
			//my_printf("\n\rSpeed: 100Mbps");
		}
	}
/*____________________________________________________________________________________*/
	void W5500_Select(void) {
			HAL_GPIO_WritePin(SPI2_NSS_ETH_GPIO_Port, SPI2_NSS_ETH_Pin, GPIO_PIN_RESET);
	}
/*____________________________________________________________________________________*/
	void W5500_Deselect(void) {
			HAL_GPIO_WritePin(SPI2_NSS_ETH_GPIO_Port, SPI2_NSS_ETH_Pin, GPIO_PIN_SET);
	}
/*____________________________________________________________________________________*/
	void W5500_ReadBuff(uint8_t *buff, uint16_t len) 
	{
			HAL_SPI_Receive(&hspi2, buff, len, HAL_MAX_DELAY);
	}
/*____________________________________________________________________________________*/
	void W5500_WriteBuff(uint8_t *buff, uint16_t len) {
			HAL_SPI_Transmit(&hspi2, buff, len, HAL_MAX_DELAY);
	}
/*____________________________________________________________________________________*/
	uint8_t W5500_ReadByte(void)
	{
		uint8_t byte;
		W5500_ReadBuff(&byte,sizeof(byte));
		return byte;
	}
/*____________________________________________________________________________________*/
	void W5500_WriteByte(uint8_t byte)
	{
		W5500_WriteBuff(&byte,sizeof(byte));
	}
/*____________________________________________________________________________________*/
	void w5500_Init(void){
		//my_printf("\r\nStart New Session!\r\n");
		
		gwIZNETINFO.mac[0] = AP_Infor_cache.IP1;
		gwIZNETINFO.mac[1] = AP_Infor_cache.IP2;
		gwIZNETINFO.mac[2] = AP_Infor_cache.IP3;
		gwIZNETINFO.mac[3] = AP_Infor_cache.IP4;
		gwIZNETINFO.mac[4] = (AP_Infor_cache.port >> 8) & 0xFF;
		gwIZNETINFO.mac[5] = AP_Infor_cache.port & 0xFF;
		
		gwIZNETINFO.ip[0] = AP_Infor_cache.IP1;
		gwIZNETINFO.ip[1] = AP_Infor_cache.IP2;
		gwIZNETINFO.ip[2] = AP_Infor_cache.IP3;
		gwIZNETINFO.ip[3] = AP_Infor_cache.IP4;
		
		des_ip[0] = AP_Infor_cache.des_IP1;
		des_ip[1] = AP_Infor_cache.des_IP2;
		des_ip[2] = AP_Infor_cache.des_IP3;
		des_ip[3] = AP_Infor_cache.des_IP4; 
		
		des_port = AP_Infor_cache.des_port;

		HAL_GPIO_WritePin(RESET_ETH_GPIO_Port, RESET_ETH_Pin, GPIO_PIN_RESET);
		HAL_Delay(5);																														// sua ngay 25/3/2025 tai KV3
		HAL_GPIO_WritePin(RESET_ETH_GPIO_Port, RESET_ETH_Pin, GPIO_PIN_SET);
		HAL_Delay(100);

		reg_wizchip_cs_cbfunc(W5500_Select, W5500_Deselect);
		reg_wizchip_spi_cbfunc(W5500_ReadByte, W5500_WriteByte);
		reg_wizchip_spiburst_cbfunc(W5500_ReadBuff, W5500_WriteBuff);
					
		wizchip_init(rx_tx_buff_sizes, rx_tx_buff_sizes);
		wizchip_setnetinfo(&gwIZNETINFO);
 }
/*____________________________________________________________________________________*/
	uint8_t w5500_Config(void){
		uint8_t result = 0;
		ctlnetwork(CN_SET_NETINFO, (void*) &gwIZNETINFO);
		HAL_Delay(100);																						// sua ngay 25/3/2025 tai KV3
		if(ctlwizchip(CW_INIT_WIZCHIP, (void*)memsize) == -1)
		{
			result = 0;
		}
		else{
			result = 1;
		}
		PHYStatusCheck();
		PrintPHYConf();
		return result;
	}
