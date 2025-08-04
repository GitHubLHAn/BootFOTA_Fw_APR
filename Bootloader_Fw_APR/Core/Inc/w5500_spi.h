/*
 * USER.h
 * Created on: 21-Mar-2024
 * Author: Le Huu An
 */

#ifndef W5500_SPI_H_
#define W5500_SPI_H_

#include "main.h"



/*DEFINE*/

#define FAIL 0xE0
#define SUCCESS 0xA0

/************************************************************************************/
/*DECLARE STRUCT*/
		
/*DECLARE FUNCTION*/
	void w5500_Init(void);
	uint8_t w5500_Config(void);
//	uint8_t creat_a_socket(void);
//	uint8_t connect_toServer(void);


/************************************************************************************/
#endif /* HC12_H */


