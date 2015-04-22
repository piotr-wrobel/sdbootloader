#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#ifndef __UART_H
	#define __UART_H

	#define GLOBAL_CONFIG "config.h"

	#ifdef GLOBAL_CONFIG
		#include GLOBAL_CONFIG
	#else //Uncoment this section for local settings
		// Ustawienia uart
		//#define BAUD 57600
	#endif


	#define BAUD_PRESCALE ((F_CPU + BAUD * 8L) / (BAUD * 16L) - 1)

	char po_konwersji[5];

	void UARTInit(void);
	uint8_t UARTSendString_P(const uint8_t *FlashLoc);
	uint8_t UARTSendString(char *napis);
	void UARTSendByte( const char Data );
	uint8_t UARTReadByte(void);

#endif
