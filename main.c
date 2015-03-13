/*

                  „Widzimy daleko, bo stoimy na ramionach olbrzyma”     
				  
				                        
 Program ten to rozwiniêta i poprawiona przez mlodedrwale wersja programu zamieszczonego na:
 http://www.elektroda.pl/rtvforum/topic1698801-0.html 
          

*/





///////////////////////////////////////////////////////////////////////////////////
// Maciej Kucia Kraków 2010 sProgATM8                                            //
//                                                                               //
// Licencja MIT                                                                  //
///////////////////////////////////////////////////////////////////////////////////



#define GLOBAL_CONFIG "config.h"

#ifdef GLOBAL_CONFIG
	#include GLOBAL_CONFIG
#else //Uncoment this section for local settings
	//#define UART_DEBUG
#endif


#ifdef UART_DEBUG
	#include "uart.h"
#endif

#define BAUD_PRESCALE ((F_CPU + BAUD * 8L) / (BAUD * 16L) - 1) 





#ifndef UCSRA
    #define UCSRA UCSR0A
#endif

#ifndef UCSRB
    #define UCSRB UCSR0B
#endif

#ifndef UCSRC
    #define UCSRC UCSR0C
#endif

#ifndef UDRE
    #define UDRE UDRE0
#endif

#ifndef UDR
    #define UDR UDR0
#endif

#ifndef RXC
    #define RXC RXC0
#endif

#ifndef UBRRL
    #define UBRRL UBRR0L
#endif
#ifndef UBRRH
    #define UBRRH UBRR0H
#endif

#ifndef RXEN
    #define RXEN RXEN0
#endif
#ifndef TXEN
    #define TXEN TXEN0
#endif

#ifndef UCSZ1
    #define UCSZ1 UCSZ01
#endif
#ifndef UCSZ0
    #define UCSZ0 UCSZ00
#endif

#ifndef URSEL
       #define URSEL URSEL01
#endif       





//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Biblioteki
#include <avr/boot.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <util/delay.h>

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Pozwala wykonaæ skok do kodu po zakoñczeniu pracy bootloadera
static void (*jump_to_app)(void) = 0x0000;

//Wysy³a bajt przez uart
static void SendByte( const char Data )
{
	while( !( UCSRA & (1<<UDRE) ) );
	UDR = Data;
}

//Odbiera bajt przez uart
static uint8_t ReadByte(void)
{
	while ( !( UCSRA & (1<<RXC) ) );
	return UDR;
}

static uint8_t ReadByteWait(void){
		uint8_t Timeout=200;
		do{
		if ( UCSRA & 1 << RXC )
            return UDR;
             _delay_ms(10);
	} while ( --Timeout );
	return 0xFF;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Ró¿noœci zwi¹zane z pozycj¹ bootloadera w pamiêci
static void skip(void) __attribute__ ((naked, section (".vectors") ));
static void skip(void)
{
    asm volatile("rjmp setup");
}

static void setup(void) __attribute__ ((naked, used, section (".init2") ));
static void setup(void)
{
	

	//Inicjacja stosu i rejestru statusu, daje równie¿ czas dla uart na inicjalizacjê
    asm volatile ( "clr __zero_reg__" );
    SREG = 0;   // Ustawiamy rej statusu
    SP = RAMEND; // i wsk stosu
} 
// Wylaczenie WatchDoga
#ifdef WDIF 
    static void __init3( 
        void ) 
        __attribute__ (( section( ".init3" ), naked, used )); 
    static void __init3( 
        void ) 
    { 
        /* wy³¹czenie watchdoga (w tych mikrokontrolerach, w których watchdog 
         * ma mo¿liwoœæ generowania przerwania pozostaje on te¿ aktywny po 
         * resecie) */ 
        MCUSR = 0; 
        WDTCSR = 1 << WDCE | 1 << WDE; 
        WDTCSR = 0; 
    } 
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
 uint8_t main(void) __attribute__ ((naked, section (".init9"),  used, noreturn )); 
 uint8_t main(void)
{
//W³¹cz uart
	UBRRL = BAUD_PRESCALE;
	UBRRH = (BAUD_PRESCALE >> 8);
	UCSRB |= (1<<RXEN)|(1<<TXEN); 	
	_delay_ms(10);
	
	
	// ^ dane:8 bitów, 1 bit parzystoœci

	// Wysy³amy znak zachêty i czekamy na odpowiedz
	SendByte( '?' );


	//Sprawdzenie je¿eli nie ma odpowiedzi lub ró¿ni siê ona od 's' to bootloader skacze do programu

	
	
	uint8_t test = ReadByteWait();
	uint8_t low,high,data;
	if( test == 's')
	{
		//Wysy³amy nag³ówek
		SendByte( 0xA0 ); // Oznaczenie wersji bootloadera
		SendByte( SIGN ); // Oznaczenie uk³adu: ATMega32
		SendByte( boot_lock_fuse_bits_get( GET_LOW_FUSE_BITS  ) );  //Fusebity
		SendByte( boot_lock_fuse_bits_get( GET_HIGH_FUSE_BITS ) );
		SendByte( boot_lock_fuse_bits_get( GET_LOCK_BITS      ) );
		
		uint8_t command;
		//G³ówna pêtla
		while ( 1 )
		{
			// Odczyt rozkazu 
			command = ReadByte();
			
			// === ZAPIS PAMIÊCI FLASH ===
			if (command=='F')
			{
				 uint8_t crc = 0xFF;
				// odczekanie do zakoñczenia dzia³añ na EEPROM i flash 
				boot_spm_busy_wait();
				
				// Odebranie adresu
				low  = ReadByte();
				crc^=low;
				high = ReadByte();
				crc^=high;
				uint16_t pageAdress =  ( uint16_t )low + ( high <<8 );

				if( pageAdress >= BLS_START ) break;
				// Skasowanie strony
				boot_page_erase( pageAdress );
				boot_spm_busy_wait();
					
				//Znak gotowoœci na dane
				SendByte( '>' );
					
				//Zape³niamy bufor strony
				for ( uint16_t Byte_Address = 0; Byte_Address < SPM_PAGESIZE; Byte_Address += 2 )
				{
					// przygotowanie 2 bajtowej instrukcji, obliczenie chksum i zapisanie bufora
					low  = ReadByte();
					high = ReadByte();
					crc^=low;
					crc^=high;
					uint16_t Instruction = (uint16_t)low + ( high << 8 );
					boot_page_fill( Byte_Address, Instruction );
				}
				// Wys³anie sumy kontrolnej w celu weryfikacji
				SendByte(crc);
				// Je¿eli weryfikacja siê powiod³a to zapisujemy
				if(ReadByte()=='k')
				{
					//Zapisane strony
					boot_page_write( pageAdress );
					boot_spm_busy_wait();
				}
				
			}else //Koniec zapisu flash
			
			
			// === ODCZYT Strony FLASH ===
			if(command=='f')
			{
				low=ReadByte();
				high=ReadByte();
				uint16_t address = ( ( uint16_t )low + (high<<8));
				data = SPM_PAGESIZE;
				while(data--)
				{
					SendByte( pgm_read_byte_near( ( uint8_t *)(address-data ) ));
					
				}
			}else
		
			// === ODCZYT BAJTU EEPROM ===
			if(command=='e')
			{
				
				low=ReadByte();
				high=ReadByte();
				SendByte(eeprom_read_byte((uint8_t *)(( uint16_t)low + (high<<8))) );
			}else
			
			// === ZAPIS BAJTU EEPROM ===
			if(command=='E')
			{
				low=ReadByte();
				high=ReadByte();
				data=ReadByte();
				eeprom_write_byte((uint8_t *)(( uint16_t)low + (high<<8)), data );
			}else
			
			break;
		}
	}
	SendByte( 'Q' );
	UCSRB = 0;
	jump_to_app(); 
	while(1);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//EOF