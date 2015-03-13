#include "uart.h"


void UARTInit(void)
{
	UBRR0L = BAUD_PRESCALE;
	UBRR0H = (BAUD_PRESCALE >> 8);
	UCSR0B |= (1<<RXEN0)|(1<<TXEN0); 	
	_delay_ms(10);
}

uint8_t UARTSendString_P(const uint8_t *FlashLoc)
{
    uint8_t cnt = 0;
	register char znak;
	while (znak=pgm_read_byte(FlashLoc++))
	{
		UARTSendByte(znak);
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


//Wysyła bajt przez uart
void UARTSendByte( const char Data )
{
	while( !( UCSR0A & (1<<UDRE0) ) );
	UDR0 = Data;
}

//Odbiera bajt przez uart
uint8_t UARTReadByte(void)
{
	while ( !( UCSR0A & (1<<RXC0) ) );
	return UDR0;
}