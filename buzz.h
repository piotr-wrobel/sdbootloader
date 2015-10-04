#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#ifndef __BUZZ_H
	#define __BUZZ_H

	//BUZZER
	#define BUZZ (1<<PD4)
	#define BUZZ_DDR DDRD
	#define BUZZ_PORT PORTD
	
	void buzzDebug(uint8_t l_sygnalow, uint8_t s_sygnalow, uint8_t s_trwa);
#endif
