// Obsługa kart SD w projekcie SDBootloader bazuje na poradniku 
// Simple FAT and SD Tutorial (http://codeandlife.com/2012/04/02/simple-fat-and-sd-tutorial-part-1/)
// modyfikacje pvg, licencja GPL 3.0

#include <avr/io.h>
#include <util/delay.h>
#ifndef __SD_SPI_H
	#define __SD_SPI_H
	#define GLOBAL_CONFIG "config.h"

	#ifdef GLOBAL_CONFIG
		#include GLOBAL_CONFIG
	#else //Uncoment this section for local settings
		// #define SD_DEBUG
	#endif

	#ifdef SD_DEBUG
		#include "uart.h"
	#endif

	#define CS_ENABLE() (CS_PORT &= ~CS)
	#define CS_DISABLE() (CS_PORT |= CS)
	#define SPI_read() SPI_write(0xFF)

	uint32_t sd_sector;
	uint16_t sd_pos;
	
	void SD_PWR_ON(void);
	void SD_PWR_OFF(void);
	void SD_release(void);
	char SD_init(void);
	void SPI_init(void);
	uint8_t SPI_write(uint8_t ch);
	uint8_t SD_command(uint8_t cmd, uint32_t arg, uint8_t read_and_skip_crc7);
	int8_t SD_read(uint32_t sector, uint16_t offset, volatile char * buffer, uint16_t len);
#endif