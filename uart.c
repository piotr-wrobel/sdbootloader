#include "uart.h"


void UARTInit(void)
{
	UBRR0L = BAUD_PRESCALE;
	UBRR0H = (BAUD_PRESCALE >> 8);
	UCSR0B |= (1<<RXEN0)|(1<<TXEN0); 	
	_delay_ms(10);
}

//Wysyła bajt przez uart
static void UARTSendByte( const char Data )
{
	while( !( UCSR0A & (1<<UDRE0) ) );
	UDR0 = Data;
}

// //Odbiera bajt przez uart
// static uint8_t UARTReadByte(void)
// {
	// while ( !( UCSR0A & (1<<RXC0) ) );
	// return UDR0;
// }

uint8_t UARTSendString_P(const uint8_t *FlashLoc)
{
    uint8_t cnt = 0;
	while(pgm_read_byte(FlashLoc))
	{
		 UARTSendByte(pgm_read_byte(FlashLoc++));
		 cnt++;
	}
	return cnt;
}

uint8_t UARTSendString(char *napis)
{
    uint8_t cnt = 0;
	while (napis[cnt])
		UARTSendByte(napis[cnt++]);
	return cnt;
}

void UARTuitoa(uint16_t liczba, char *string)
{
	uint8_t nibble=0,pozycja;
	for(pozycja=0;pozycja<4;pozycja++)
	{
		nibble=(liczba>>12);
		liczba=(liczba<<4);
		if(nibble <= 9)
			nibble+=48;
		else
			nibble+=55;
		string[pozycja]=nibble;
	}
	string[pozycja]=0;
}

void UARTprintPage(uint16_t page_begin,uint16_t page_end,char *string, uint8_t real_programing)
{
	UARTuitoa(page_begin,string);
	UARTSendString("\r\nPAGE: 0x");
	UARTSendString(string);
	UARTuitoa(page_end,string);
	UARTSendString(" -> 0x");
	UARTSendString(string);
	if(!real_programing)
		UARTSendString(" NP");
}
