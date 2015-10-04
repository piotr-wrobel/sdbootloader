#ifndef __CONFIG_H
	#define __CONFIG_H

	#define BUZZ_DEBUG
	#define UART_DEBUG
	#define REAL_PROGRAMING

	#ifdef UART_DEBUG
		//#define DEBUG
	#endif

	// SPI config
	#define CS (1<<PB2)
	#define CS_PORT PORTB
	#define CS_DDR DDRB
	#define MOSI (1<<PB3)
	#define MISO (1<<PB4)
	#define SCK (1<<PB5)
	
	// BOOTLOADER BUTTON
	#define DDR_BUTT	DDRD
	#define PORT_BUTT	PORTD
	#define PIN_BUTT	PIND
	#define BUTTON		PD5


	#define BUZZ_ZERO_LONG 0
	#define BUZZ_ONE_LONG 1
	#define BUZZ_SD_ERR 1
	#define BUZZ_FAT_ERR 2
	#define BUZZ_FILE_ERR 3
	#define BUZZ_CONTROL_SUMM_ERR 4
	#define BUZZ_VERIF_ERR 5
	#define BUZZ_END 0
	#define BUZZ_BEGIN 0

	// UART config
	//#define BAUD 57600

	//File config
	#define FILE_NAME "ALD_UPD " //Musi byc 8 znakow, jesli mniej, to uzupelnij spacją
	#define FILE_EXT "HEX"

#endif