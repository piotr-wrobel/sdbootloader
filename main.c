/*

                  �Widzimy daleko, bo stoimy na ramionach olbrzyma�     
				  
				                        
 Program ten to rozwini�ta i poprawiona przez mlodedrwale wersja programu zamieszczonego na:
 http://www.elektroda.pl/rtvforum/topic1698801-0.html 
          

*/





///////////////////////////////////////////////////////////////////////////////////
// Maciej Kucia Krak�w 2010 sProgATM8                                            //
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

#define ASCII_DWUKROPEK 0x3A
#define IHEX_RLEN_BEGIN 1
#define IHEX_RADRESS_BEGIN 3
#define IHEX_RTYPE_BEGIN 7
#define IHEX_DATA_BEGIN 9



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
#include <stdlib.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>

#include "sd_spi.h"
#include "fat16.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Pozwala wykona� skok do kodu po zako�czeniu pracy bootloadera
static void (*jump_to_app)(void) = 0x0000;


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// R�no�ci zwi�zane z pozycj� bootloadera w pami�ci
// static void skip(void) __attribute__ ((naked, section (".vectors") ));
// static void skip(void)
// {
    // asm volatile("rjmp setup");
// }

static void setup(void) __attribute__ ((naked, used, section (".init2") ));
static void setup(void)
{
	

	//Inicjacja stosu i rejestru statusu, daje r�wnie� czas dla uart na inicjalizacj�
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
        /* wy��czenie watchdoga (w tych mikrokontrolerach, w kt�rych watchdog 
         * ma mo�liwo�� generowania przerwania pozostaje on te� aktywny po 
         * resecie) */ 
        MCUSR = 0; 
        WDTCSR = 1 << WDCE | 1 << WDE; 
        WDTCSR = 0; 
    } 
#endif

#ifdef BUZZ_DEBUG
static void buzzDebug(uint8_t sygnalow)
{
	BUZZ_PORT |= BUZZ;
	_delay_ms(700);
	BUZZ_PORT &=~ BUZZ;
	_delay_ms(700);
	for(uint8_t i=0; i<sygnalow; i++)
	{
		BUZZ_PORT |= BUZZ;
		_delay_ms(300);
		BUZZ_PORT &=~ BUZZ;
		_delay_ms(300);
	}
}
#endif

static void small_itoa(uint16_t liczba, char* string, uint8_t podstawa)
{
	uint8_t pozycja=0;
	uint8_t nibble=0;
	while(liczba)
	{
		nibble=liczba & 0xF;
		liczba=(liczba>>4);
		if(nibble <= 9)
			string[pozycja]=nibble+30;
		else
			string[pozycja]=nibble+65;
		pozycja++;
	}
	string[pozycja]=0;
}

static unsigned int small_strtol( char *v, size_t len, long *l ){
     register size_t i;
     register long n = 0;
     register char c;
     int sign = 1;
     int start = 0;
 	
     // check we have a negative sign first
     if( v[0] == '-' ){
         sign = -1;
         start = 1;
     }
 
     for( i = start; i < len; ++i ){
         c = v[i];
         if( c >= '0' && c <= '9' ){
             n = ( n * 10 ) + ( c - '0' );
         }
         else
             return 0;
     }
 
     *l = n * sign;
 
     return 1;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
int main(void) __attribute__ ((section (".init9"),  used, noreturn )); 
int main(void)
{
	char ret;
	#ifdef BUZZ_DEBUG
		BUZZ_DDR |= BUZZ;
		BUZZ_PORT &=~ BUZZ;
	#endif
	#ifdef UART_DEBUG	
		UARTInit();
	#endif

	SPI_init();
    
	ret = SD_init();
	if(ret) 
	{
		#ifdef BUZZ_DEBUG
			buzzDebug(BUZZ_SD_ERR);
		#endif
		jump_to_app();
    }
	
    ret = fat16_init();
	if(ret) 
	{
		#ifdef BUZZ_DEBUG
			buzzDebug(BUZZ_FAT_ERR);
		#endif
		jump_to_app();
    }

	ret = fat16_open_file(FILE_NAME,FILE_EXT);       
    if(ret) 
	{
		#ifdef BUZZ_DEBUG
			buzzDebug(BUZZ_FILE_ERR);
		#endif
        jump_to_app();
    }
	
	
	
	uint8_t rindex=0,bajtow_w_rekordzie=0, typ_rekordu=0, suma_kontrolna=0;
	uint8_t	suma_kontrolna_odczytana=0, bajt_danych=0, starszy_bajt_danych=0, koniec_danych=0,index_slowa=0;
	uint16_t adres=0,pageAdress=0xFFFF,Byte_Address=0;
	char bufor[8];
	uint8_t bufor_strony[SPM_PAGESIZE];
	boot_spm_busy_wait();
	while(fat16_state.file_left) 
	{
		ret = fat16_read_file(FAT16_BUFFER_SIZE);
		for(uint8_t i=0; i<ret; i++) //Jedziemy z naszym FAT16_BUFFER_SIZE buforkiem :)
        {
			if(!rindex && fat16_buffer[i]==ASCII_DWUKROPEK) //Mamy nowa linie
			{
				rindex++;
			}else if(rindex && rindex<IHEX_RADRESS_BEGIN) //Do bufora dwa znaki dlugosci rekordu
			{
				bufor[rindex-IHEX_RLEN_BEGIN]=fat16_buffer[i];
				rindex++;
			}else if(rindex && rindex<IHEX_RTYPE_BEGIN) //Do bufora 4 znakowy adres
			{
				if(rindex==IHEX_RADRESS_BEGIN) //Konczymy string w buforze i zamieniamy na ilosc bajtow danych w linii
				{	
					bufor[rindex-IHEX_RLEN_BEGIN]=0;
					bajtow_w_rekordzie=strtol(bufor,NULL,16);
					//suma_kontrolna=bajtow_w_rekordzie; //Pierszy bajt do sumu kontrolnej
				}
				if(bajtow_w_rekordzie) //Skladamy 4 znakowy adres
				{
					bufor[rindex-IHEX_RADRESS_BEGIN]=fat16_buffer[i]; //Zapelniamy bufor nowymi znakami
					koniec_danych=((bajtow_w_rekordzie*2)+IHEX_DATA_BEGIN); //Do ktorego znaku w rekordzie siegaja dane ?
				}else
					koniec_danych=0;
				rindex++;
			}else if(rindex && bajtow_w_rekordzie && rindex < IHEX_DATA_BEGIN ) //Do bufora dwuznakowy typ rekordu
			{
				if(rindex==IHEX_RTYPE_BEGIN) //Konczymy string w buforze i zamienimy na 2 bajtowy adres
				{
					bufor[rindex-IHEX_RADRESS_BEGIN]=0;
					adres=strtol(bufor,NULL,16);
					if(Byte_Address > 0 && (pageAdress + Byte_Address) != adres) //Mamy juz cos w buforze, a nowy adres rekordu nie jest kontynuacja
					{
						#ifdef UART_DEBUG
							small_itoa(pageAdress + Byte_Address,po_konwersji,16);
							UARTSendString("\r\nA1 <> A2: 0x");
							UARTSendString(po_konwersji);
							small_itoa(adres,po_konwersji,16);
							UARTSendString(" <> 0x");
							UARTSendString(po_konwersji);							
						#endif
						Byte_Address+=2;
						while(Byte_Address < SPM_PAGESIZE)
						{
							bufor_strony[Byte_Address]=0xFF;
							bufor_strony[Byte_Address+1]=0xFF;
							//boot_page_fill( Byte_Address, 0xFFFF );
							Byte_Address += 2;							
						}
						
						for(uint8_t bufor_index=0; bufor_index < SPM_PAGESIZE; bufor_index+=2)
							boot_page_fill( bufor_index, bufor_strony[bufor_index] );
						#ifdef REAL_PROGRAMING
							boot_page_erase( pageAdress ); //kasujemy stron�
							boot_spm_busy_wait();
							boot_page_write( pageAdress ); //zapisujemy strone nowymi danymi
							boot_spm_busy_wait();
						#endif
						// Tu mozna dorobic weryfikacje zapisu w oparciu o bufor
						//Co tu dalej ? trzeba sie zastanowic
					}
					//To ponizej, tez nie jestem pewien, czy prawidlowo tutaj sie znajduje - do sprawdzenia.
					if(pageAdress==0xFFFF)
						pageAdress=adres; //Pierwszy adres powinien byc poczatkiem strony - p�ki co brak walidacji
					
					suma_kontrolna+=(adres&0x00FF); //Mlodszy bajt adresu dodajemy do sumy kontrolnej
					suma_kontrolna+=(adres>>8);		//Starszy bajt adresu dodajemy do sumy kontrolnej
				}
				bufor[rindex-IHEX_RTYPE_BEGIN]=fat16_buffer[i]; //Zapelniamy bufor nowymi znakami
				rindex++;
			}else if(rindex && bajtow_w_rekordzie && rindex < koniec_danych) //Tu bedziemy czytac dane
			{
				if(rindex==IHEX_DATA_BEGIN) //Konczymy string w buforze i zamienimy na typ rekordu
				{
					bufor[rindex-IHEX_RTYPE_BEGIN]=0;
					typ_rekordu=strtol(bufor,NULL,16);
					suma_kontrolna+=typ_rekordu; //Typ rekordu dodajemy do sumy kontrolnej
				}					
				uint8_t indeks=(rindex+1)&0x01; //Indeks bedzie 0 i 1 na przemian, bo chcemy czytac jednobajtow_w_rekordziee wartosci hex(dwuznakowe)
				bufor[indeks]=fat16_buffer[i]; //Zapelniamy bufor nowymi znakami
				if(indeks && !typ_rekordu) //Drugi znak w buforze, zamieniamy na bajt danych, jesli ten rekord to rekord z danymi
				{
					bufor[indeks+1]=0;
					bajt_danych=strtol(bufor,NULL,16); //Mamy gotowy bajt danych
					if(index_slowa)
					{
						starszy_bajt_danych=bajt_danych; //Starszy bajt do slowa
						index_slowa++;
					}else
					{
						index_slowa=0;
						bufor_strony[Byte_Address]=starszy_bajt_danych;
						bufor_strony[Byte_Address+1]=bajt_danych;
						Byte_Address += 2;
						if(Byte_Address >= SPM_PAGESIZE) //Bufor strony gotowy
						{
							#ifdef UART_DEBUG
								small_itoa(pageAdress,po_konwersji,16);
								UARTSendString("\r\nPa: 0x");
								UARTSendString(po_konwersji);
							#endif
							
							for(uint8_t bufor_index=0; bufor_index < SPM_PAGESIZE; bufor_index+=2)
								boot_page_fill( bufor_index, bufor_strony[bufor_index] );
							#ifdef REAL_PROGRAMING
								boot_page_erase( pageAdress ); //kasujemy stron�
								boot_spm_busy_wait();
								boot_page_write( pageAdress ); //zapisujemy strone nowymi danymi
								boot_spm_busy_wait();
							#endif
							// Tu mozna dorobic weryfikacje zapisu w oparciu o bufor
							Byte_Address=0;
							pageAdress=adres+((rindex-IHEX_DATA_BEGIN)>>1)+1; // Ustalamy nowy adres strony danych (odczytany z ostatniego rekordu ihex plus juz wykorzystane dane z rekordu)
							#ifdef UART_DEBUG
								small_itoa(pageAdress,po_konwersji,16);
								UARTSendString("\r\nNp: 0x");
								UARTSendString(po_konwersji);
							#endif
						}
					}
					suma_kontrolna+=bajt_danych; //Wszystkie bajty danych po kolei dodawane do sumy kontrolnej
					
				}
				rindex++;
			}else if(rindex && bajtow_w_rekordzie && rindex<koniec_danych+2) //Tu juz czytamy ostatnie dwa znaki sumy kontrolnej
			{
				if(rindex==koniec_danych)
				{
					bufor[0]=fat16_buffer[i];
				}else
				{
					bufor[1]=fat16_buffer[i];
					bufor[2]=0;
					suma_kontrolna_odczytana=strtol(bufor,NULL,16);
					suma_kontrolna=0x100-suma_kontrolna; //Kodem uzupelnien do 2
					if(suma_kontrolna!=suma_kontrolna_odczytana) //Mamy prawidlowy rekord
					{
						#ifdef UART_DEBUG
							small_itoa(adres,po_konwersji,16);
							UARTSendString(po_konwersji);
						#endif
						#ifdef BUZZ_DEBUG
							buzzDebug(BUZZ_CONTROL_SUMM_ERR);
						#endif
						jump_to_app();
					}
				}
				rindex++;

			} else
			{
				rindex=0;
			}
		}
    }


















// Stara wersja bootloadera - do wykorzystania !

	// //W��cz uart
	// UBRRL = BAUD_PRESCALE;
	// UBRRH = (BAUD_PRESCALE >> 8);
	// UCSRB |= (1<<RXEN)|(1<<TXEN); 	
	// _delay_ms(10);
	
	
	// // ^ dane:8 bit�w, 1 bit parzysto�ci

	// // Wysy�amy znak zach�ty i czekamy na odpowiedz
	// SendByte( '?' );


	// //Sprawdzenie je�eli nie ma odpowiedzi lub r�ni si� ona od 's' to bootloader skacze do programu

	
	
	// uint8_t test = ReadByteWait();
	// uint8_t low,high,data;
	// if( test == 's')
	// {
		// //Wysy�amy nag��wek
		// SendByte( 0xA0 ); // Oznaczenie wersji bootloadera
		// SendByte( SIGN ); // Sygnatura procesora z Makefile
		// SendByte( boot_lock_fuse_bits_get( GET_LOW_FUSE_BITS  ) );  //Fusebity
		// SendByte( boot_lock_fuse_bits_get( GET_HIGH_FUSE_BITS ) );
		// SendByte( boot_lock_fuse_bits_get( GET_LOCK_BITS      ) );
		
		// uint8_t command;
		// //G��wna p�tla
		// while ( 1 )
		// {
			// // Odczyt rozkazu 
			// command = ReadByte();
			
			// // === ZAPIS PAMI�CI FLASH ===
			// if (command=='F')
			// {
				 // uint8_t crc = 0xFF;
				// // odczekanie do zako�czenia dzia�a� na EEPROM i flash 
				// boot_spm_busy_wait();
				
				// // Odebranie adresu
				// low  = ReadByte();
				// crc^=low;
				// high = ReadByte();
				// crc^=high;
				// uint16_t pageAdress =  ( uint16_t )low + ( high <<8 );

				// if( pageAdress >= BLS_START ) break;
				// // Skasowanie strony
				// boot_page_erase( pageAdress );
				// boot_spm_busy_wait();
					
				// //Znak gotowo�ci na dane
				// SendByte( '>' );
					
				// //Zape�niamy bufor strony
				// for ( uint16_t Byte_Address = 0; Byte_Address < SPM_PAGESIZE; Byte_Address += 2 )
				// {
					// // przygotowanie 2 bajtowej instrukcji, obliczenie chksum i zapisanie bufora
					// low  = ReadByte();
					// high = ReadByte();
					// crc^=low;
					// crc^=high;
					// uint16_t Instruction = (uint16_t)low + ( high << 8 );
					// boot_page_fill( Byte_Address, Instruction );
				// }
				// // Wys�anie sumy kontrolnej w celu weryfikacji
				// SendByte(crc);
				// // Je�eli weryfikacja si� powiod�a to zapisujemy
				// if(ReadByte()=='k')
				// {
					// //Zapisane strony
					// boot_page_write( pageAdress );
					// boot_spm_busy_wait();
				// }
				
			// }else //Koniec zapisu flash
			
			
			// // === ODCZYT Strony FLASH ===
			// if(command=='f')
			// {
				// low=ReadByte();
				// high=ReadByte();
				// uint16_t address = ( ( uint16_t )low + (high<<8));
				// data = SPM_PAGESIZE;
				// while(data--)
				// {
					// SendByte( pgm_read_byte_near( ( uint8_t *)(address-data ) ));
					
				// }
			// }else
		
			// // === ODCZYT BAJTU EEPROM ===
			// if(command=='e')
			// {
				
				// low=ReadByte();
				// high=ReadByte();
				// SendByte(eeprom_read_byte((uint8_t *)(( uint16_t)low + (high<<8))) );
			// }else
			
			// // === ZAPIS BAJTU EEPROM ===
			// if(command=='E')
			// {
				// low=ReadByte();
				// high=ReadByte();
				// data=ReadByte();
				// eeprom_write_byte((uint8_t *)(( uint16_t)low + (high<<8)), data );
			// }else
			
			// break;
		// }
	// }
	// SendByte( 'Q' );
	// UCSRB = 0;
	// jump_to_app(); 
	// while(1);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//EOF