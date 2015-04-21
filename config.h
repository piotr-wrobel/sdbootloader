

//#define UART_DEBUG

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


// UART config
//#define BAUD 57600

//File config
#define FILE_NAME "ALD_UPD " //Musi byc 8 znakow, jesli mnie, to uzupelnil spacją
#define FILE_EXT "HEX"

