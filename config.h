#ifndef __CONFIG_H
	#define __CONFIG_H
	
	//#define BUZZ_DEBUG
	#define UART_DEBUG
	//#define SD_DEBUG
	//#define FAT_DEBUG
	#define CODE_DEBUG
	#define REAL_PROGRAMING


	// SPI config
	#define CS (1<<PC3)
	#define CS_PORT PORTC
	#define CS_DDR DDRC
	#define MOSI (1<<PB3)
	#define MISO (1<<PB4)
	#define SCK (1<<PB5)

	//SD commands
	#define SD_GO_IDLE 0
	#define SD_SEND_OP_COND 1
	#define SD_SEND_IF_COND 8
	#define SD_SET_BLOCK_LEN 16
	#define SD_READ_SINGLE_BLOCK 17
	#define SD_APP_CMD 55
	#define SD_READ_OCR 58
	#define SD_CRC_ON_OFF 59

	#define SD_SEND_APP_OP 41

	#define CRC_OFF 0
	#define CRC_ON 1
	#define SD_BLOCK_LEN 512

	#define SKIPCRC7 1
	#define NOSKIPCRC7 0
	
	// BOOTLOADER BUTTON
	#define DDR_BUTT	DDRD
	#define PORT_BUTT	PORTD
	#define PIN_BUTT	PIND
	#define BUTT_MENU	PD5
	#define BUTT_SET	PD6


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
	
	#define CODE_MASK 0xC0
	

#endif