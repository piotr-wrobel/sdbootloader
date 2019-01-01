//
// SDBootloader bazuje na kodzie projektu
// http://www.mlodedrwale.pl/2013/06/09/bootlader-avr-ferret/ 
// dla procesorów AVR, oryginalna licencja MIT, po zmianach zastąpiona zgodną GPL 3.0
// 
// Zmiany (obsługa karty SD, parsowanie formatu INTEL HEX, dekodowanie zaszyfrowanego wsadu )
// pvg
//
//
// Szyfrowanie wsadu za pomocą programu sdbootloader_encoder (https://github.com/piotr-wrobel/sdbootloader_encoder)
//

/*

                  „Widzimy daleko, bo stoimy na ramionach olbrzyma”     
				  
				                        
 Program ten to rozwinięta i poprawiona przez mlodedrwale wersja programu zamieszczonego na:
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

#ifdef BUZZ_DEBUG
	#include "buzz.h"
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



//Biblioteki
#include <avr/boot.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>

#include "sd_spi.h"
#include "fat16.h"

// const char VER_ERR[]	PROGMEM="\r\nVER ERR: 0x";
// const char STRZALKA[]	PROGMEM=" -> ";
// const char BOOTLOADER_START[]	PROGMEM="\r\nBOOTLOADER START!";
// const char SD_ERROR[]	PROGMEM="\r\nSD ERROR!";
// const char FAT_ERROR[]	PROGMEM="\r\nFAT ERROR!";
// const char FILE_ERROR[]	PROGMEM="\r\nFILE ERROR!";

//Pozwala wykonać skok do kodu po zakończeniu pracy bootloadera
static void (*jump_to_app)(void) = 0x0000;


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Różności związane z pozycją bootloadera w pamięci
static void skip(void) __attribute__ ((naked, section (".vectors") ));
static void skip(void)
{
    asm volatile("rjmp setup");
}

static void setup(void) __attribute__ ((naked, used, section (".init2") ));
static void setup(void)
{
	
	cli();
	//Inicjacja stosu i rejestru statusu, daje również czas dla uart na inicjalizację
    asm volatile ( "clr __zero_reg__" );
    SREG = 0;   // Ustawiamy rej statusu
    SP = RAMEND; // i wsk stosu
} 
// Wylaczenie WatchDoga
#ifdef WDIF //Jesli jest zdefiniowany, to w naglowkach systemowych
    static void __init3( 
        void ) 
        __attribute__ (( section( ".init3" ), naked, used )); 
    static void __init3( 
        void ) 
    { 
        /* wyłączenie watchdoga (w tych mikrokontrolerach, w których watchdog 
         * ma możliwość generowania przerwania pozostaje on też aktywny po 
         * resecie) */ 
        MCUSR = 0; 
        WDTCSR = 1 << WDCE | 1 << WDE; 
        WDTCSR = 0; 
    } 
#endif

static unsigned int hexstr2ui16( char *string, uint8_t start, uint8_t size )
{
	uint8_t i;
	char c;
	uint16_t wynik=0;
	
	for( i = start; i < start+size; i++ )
	{
		wynik=wynik<<4;
		c = string[i];
		if( c >= '0' && c <= '9' )
			c-=48;
		else //For A,B,C,D,E,F
			c-=55; 
		wynik+=c;
	}

	return wynik;
}


static void verifyPage(uint16_t pageAdress, uint8_t *bufor_strony)
{
	boot_rww_enable_safe();
	// Weryfikacja zapisanej strony
	for(uint8_t bufor_index=0; bufor_index < SPM_PAGESIZE; bufor_index++)
	{
		uint8_t odczytany_bajt=pgm_read_byte_near( ( uint8_t *)(pageAdress+bufor_index) );
		if(bufor_strony[bufor_index] != odczytany_bajt)
		{
		#ifdef BUZZ_DEBUG
			buzzDebug(BUZZ_ONE_LONG,BUZZ_VERIF_ERR,200);
		#endif
		#ifdef UART_DEBUG
			//UARTSendString_P((void *)VER_ERR);
			UARTSendString("\r\nVER ERR: 0x");
			UARTuitoa(pageAdress+bufor_index,po_konwersji);
			UARTSendString(po_konwersji);
			UARTuitoa((bufor_strony[bufor_index]<<8)+odczytany_bajt,po_konwersji);
			//UARTSendString_P((void *)STRZALKA);
			UARTSendString(" -> ");
			UARTSendString(po_konwersji);
		#endif
			while(1); //Tu lepiej zapetlic, niz skakac do programu, bo program jest uszkodzony
		}
	}
}

static void writePage(uint16_t pageAdress,uint8_t *bufor_strony,uint8_t real_programing)
{
	for(uint8_t bufor_index=0; bufor_index < SPM_PAGESIZE; bufor_index+=2)
		boot_page_fill( bufor_index, (uint16_t)(bufor_strony[bufor_index+1]<<8)+bufor_strony[bufor_index] );
#ifdef REAL_PROGRAMING
	if(real_programing)
		boot_page_erase( pageAdress ); //kasujemy stronę
	#ifdef BUZZ_DEBUG
		buzzDebug(BUZZ_ZERO_LONG,1,50);
	#endif
	boot_spm_busy_wait();
	if(real_programing)
		boot_page_write( pageAdress ); //zapisujemy strone nowymi danymi
	boot_spm_busy_wait();
#endif
}

int main(void) __attribute__ ((section (".init9"),  used, noreturn )); 
int main(void)
{
	uint8_t real_programing=0; //Od tej zmiennej zalezy, czy na serio zapisujemy flash
	uint8_t ret;
	
	DDR_BUTT &=~ ((1<<BUTT_MENU) | (1<<BUTT_SET)); // Oba jako wejscia
	PORT_BUTT |= ((1<<BUTT_MENU) | (1<<BUTT_SET)); //Rezystor podciagajacy na obu
	
	if(PIN_BUTT&(1<<BUTT_MENU))
		jump_to_app(); //Button nie jest wcisniety, od razu uciekamy z bootloadera
	_delay_ms(3000); //Czekamy 3 sekundy
	if(PIN_BUTT&(1<<BUTT_MENU))
		jump_to_app(); //Byl wcisniety, ale nie jest, uciekamy z bootloadera
	if(PIN_BUTT&(1<<BUTT_SET))
		real_programing=1;
	else
		real_programing=0;
	SD_PWR_ON(); //Zasilanie na karte SD
	_delay_ms(500); //Inicjacja karty	
#ifdef BUZZ_DEBUG
	BUZZ_DDR |= BUZZ;
	BUZZ_PORT &=~ BUZZ;
	buzzDebug(BUZZ_ONE_LONG,BUZZ_BEGIN,200);
#endif
#ifdef UART_DEBUG	
	UARTInit();
	//UARTSendString_P((void *)BOOTLOADER_START);
	UARTSendString("\r\nBOOTLOADER START!");
#endif
	
	SPI_init();
    
	ret = SD_init();
	if(ret==1)
		ret = SD_init();
	if(ret) 
	{
	#ifdef BUZZ_DEBUG
		buzzDebug(BUZZ_ONE_LONG,BUZZ_SD_ERR,200);
	#endif
	#ifdef UART_DEBUG	
		//UARTSendString_P((void *)SD_ERROR);
		UARTSendString("\r\nSD ERROR!");
	#endif	
		SD_PWR_OFF(); //Wylaczamy zasilanie SD
		jump_to_app();
    }
	
    ret = fat16_init();
	if(ret) 
	{
	#ifdef BUZZ_DEBUG
		buzzDebug(BUZZ_ONE_LONG,BUZZ_FAT_ERR,200);
	#endif
	#ifdef UART_DEBUG	
		//UARTSendString_P((void *)FAT_ERROR);
		UARTSendString("\r\nFAT ERROR!");
	#endif	
		SD_PWR_OFF(); //Wylaczamy zasilanie SD
		jump_to_app();
    }

	ret = fat16_open_file(FILE_NAME,FILE_EXT);       
    if(ret) 
	{
	#ifdef BUZZ_DEBUG
		buzzDebug(BUZZ_ONE_LONG,BUZZ_FILE_ERR,200);
	#endif
	#ifdef UART_DEBUG	
		//UARTSendString_P((void *)FILE_ERROR);
		UARTSendString("\r\nFILE ERROR!");
	#endif	
        SD_PWR_OFF(); //Wylaczamy zasilanie SD
		jump_to_app();
    }
	
	
	
	uint8_t rindex=0,bajtow_w_rekordzie=0, typ_rekordu=0, suma_kontrolna=0, klucz=0;
	uint8_t	suma_kontrolna_odczytana=0, bajt_danych=0, nbajt_danych=0,starszy_bajt_danych=0, koniec_danych=0,index_slowa=0;
	uint16_t adres=0,pageAdress=0xFFFF,Byte_Address=0,aktualny_adres=0;
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
				typ_rekordu=0xFF;
			}else if(rindex && rindex<IHEX_RADRESS_BEGIN) //Do bufora dwa znaki dlugosci rekordu
			{
				bufor[rindex-IHEX_RLEN_BEGIN]=fat16_buffer[i];
				rindex++;
			}else if(rindex && rindex<IHEX_RTYPE_BEGIN) //Do bufora 4 znakowy adres
			{
				if(rindex==IHEX_RADRESS_BEGIN) //Konczymy string w buforze i zamieniamy na ilosc bajtow danych w linii
				{	
					bufor[rindex-IHEX_RLEN_BEGIN]=0;
					bajtow_w_rekordzie=hexstr2ui16(bufor,0,2);
					suma_kontrolna=bajtow_w_rekordzie; //Pierszy bajt do sumu kontrolnej
				}

				bufor[rindex-IHEX_RADRESS_BEGIN]=fat16_buffer[i]; //Zapelniamy bufor nowymi znakami
				koniec_danych=((bajtow_w_rekordzie*2)+IHEX_DATA_BEGIN); //Do ktorego znaku w rekordzie siegaja dane ?

				rindex++;
			}else if(rindex && /*bajtow_w_rekordzie &&*/ rindex < IHEX_DATA_BEGIN ) //Do bufora dwuznakowy typ rekordu
			{
				if(rindex==IHEX_RTYPE_BEGIN) //Konczymy string w buforze i zamienimy na 2 bajtowy adres
				{
					bufor[rindex-IHEX_RADRESS_BEGIN]=0;
					adres=hexstr2ui16(bufor,0,4);
					if(pageAdress==0xFFFF)
						pageAdress=adres; //Pierwszy adres powinien byc poczatkiem strony - póki co brak walidacji
					else if(Byte_Address > 0 && (pageAdress + Byte_Address) != adres) //Mamy juz cos w buforze, a nowy adres rekordu nie jest kontynuacja
					{
					#ifdef UART_DEBUG
						UARTSendString("\r\nA1!A2:0x");
						UARTuitoa(pageAdress+Byte_Address,po_konwersji);
						UARTSendString(po_konwersji);
						UARTSendString("!0x");							
						UARTuitoa(adres,po_konwersji);
						UARTSendString(po_konwersji);							
					#endif
						Byte_Address+=2;
						while(Byte_Address < SPM_PAGESIZE)
						{
							bufor_strony[Byte_Address]=0xFF;
							bufor_strony[Byte_Address+1]=0xFF;
							Byte_Address += 2;							
						}
						
						writePage(pageAdress,bufor_strony, real_programing);
						// Weryfikacja zapisanej strony
						verifyPage(pageAdress, bufor_strony);
					#ifdef UART_DEBUG
						UARTprintPage(pageAdress,pageAdress+SPM_PAGESIZE-1,po_konwersji, real_programing);
					#endif
						pageAdress=adres;
						Byte_Address=0;
					}
					//To ponizej, tez nie jestem pewien, czy prawidlowo tutaj sie znajduje - do sprawdzenia.
					
					suma_kontrolna+=(adres&0x00FF); //Mlodszy bajt adresu dodajemy do sumy kontrolnej
					suma_kontrolna+=(adres>>8);		//Starszy bajt adresu dodajemy do sumy kontrolnej
				}
				bufor[rindex-IHEX_RTYPE_BEGIN]=fat16_buffer[i]; //Zapelniamy bufor nowymi znakami
				rindex++;
			}else if(rindex && /*bajtow_w_rekordzie &&*/ rindex < koniec_danych) //Tu bedziemy czytac dane
			{
				if(rindex==IHEX_DATA_BEGIN) //Konczymy string w buforze i zamienimy na typ rekordu
				{
					bufor[rindex-IHEX_RTYPE_BEGIN]=0;
					typ_rekordu=hexstr2ui16(bufor,0,2);
					suma_kontrolna+=typ_rekordu; //Typ rekordu dodajemy do sumy kontrolnej
				}					
				uint8_t indeks=(rindex+1)&0x01; //Indeks bedzie 0 i 1 na przemian, bo chcemy czytac jednobajtow_w_rekordziee wartosci hex(dwuznakowe)
				bufor[indeks]=fat16_buffer[i]; //Zapelniamy bufor nowymi znakami
				if(indeks) //Drugi znak w buforze, zamieniamy na bajt danych, jesli ten rekord to rekord z danymi
				{
					bufor[indeks+1]=0;
					bajt_danych=hexstr2ui16(bufor,0,2);//Mamy gotowy bajt danych
					suma_kontrolna+=bajt_danych; //Wszystkie bajty danych po kolei dodawane do sumy kontrolnej
					if(!typ_rekordu)
					{
						if(!index_slowa)
						{
						#ifdef CODE_MASK	
							aktualny_adres=pageAdress+Byte_Address;
							klucz=(aktualny_adres>>4)&0xFF;
							if((aktualny_adres&0xFF)==CODE_MASK)
							{
								nbajt_danych=((uint16_t)bajt_danych-klucz)&0xFF;
							#ifdef CODE_DEBUG		
								UARTSendString("\r\nC:");
								UARTuitoa(bajt_danych,po_konwersji);
								UARTSendString(po_konwersji);
								UARTSendString("-");
								UARTuitoa(klucz,po_konwersji);
								UARTSendString(po_konwersji);
								UARTSendString("=");
								UARTuitoa(nbajt_danych,po_konwersji);
								UARTSendString(po_konwersji);
							#endif
								bajt_danych=nbajt_danych;
							}
						#endif
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
								uint16_t oldPageAdress=pageAdress;
							#endif
								writePage(pageAdress,bufor_strony, real_programing);
								// Weryfikacja zapisanej strony
								verifyPage(pageAdress, bufor_strony);
								Byte_Address=0;
								pageAdress=adres+((rindex-IHEX_DATA_BEGIN)>>1)+1; // Ustalamy nowy adres strony danych (odczytany z ostatniego rekordu ihex plus juz wykorzystane dane z rekordu)
							#ifdef UART_DEBUG
								UARTprintPage(oldPageAdress,pageAdress-1,po_konwersji, real_programing);
							#endif
							}
						}
					}
				}
				rindex++;
			}else if(rindex && /*bajtow_w_rekordzie &&*/ rindex<koniec_danych+2) //Tu juz czytamy ostatnie dwa znaki sumy kontrolnej
			{
				if(typ_rekordu==0xFF) //Nie bylo danych i typ rekordu nie zostal jeszcze okreslony
				{
					bufor[rindex-IHEX_RTYPE_BEGIN]=0;
					typ_rekordu=hexstr2ui16(bufor,0,2);
					suma_kontrolna+=typ_rekordu; //Typ rekordu dodajemy do sumy kontrolnej
				}
				if(rindex==koniec_danych)
				{
					bufor[0]=fat16_buffer[i];
				}else
				{
					bufor[1]=fat16_buffer[i];
					bufor[2]=0;
					suma_kontrolna_odczytana=hexstr2ui16( bufor, 0, 2 );
					suma_kontrolna=0x100-suma_kontrolna; //Kodem uzupelnien do 2
					if(suma_kontrolna!=suma_kontrolna_odczytana) //Mamy nieprawidlowy rekord
					{
					#ifdef UART_DEBUG
						UARTSendString("\r\nCRC ERR:");
						UARTuitoa(adres,po_konwersji);
						UARTSendString(po_konwersji);
						UARTuitoa(suma_kontrolna,po_konwersji);
						UARTSendString(po_konwersji);
						UARTuitoa(suma_kontrolna_odczytana,po_konwersji);
						UARTSendString(po_konwersji);							
					#endif
					#ifdef BUZZ_DEBUG
						buzzDebug(BUZZ_ONE_LONG,BUZZ_CONTROL_SUMM_ERR,200);
					#endif
						while(1); //Tu lepiej zapetlic, niz skakac do programu, bo program jest uszkodzony
					}
				}
				rindex++;

			} else
			{
				rindex=0;
			}
		}
    }
#ifdef UART_DEBUG
	UARTSendString("\r\nGotowe!");
#endif
#ifdef BUZZ_DEBUG
	buzzDebug(BUZZ_ONE_LONG,BUZZ_END,200);
#endif
    SD_PWR_OFF(); //Wylaczamy zasilanie SD
	asm volatile ( "clr __zero_reg__" );
    SREG = 0;   // Ustawiamy rej statusu
    SP = RAMEND; // i wsk stosu
	sei();
	jump_to_app(); //Chyba wszystko ok, skaczemy do nowego programu :)
}
