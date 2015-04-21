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
	#include <stdlib.h>
#endif

#define BAUD_PRESCALE ((F_CPU + BAUD * 8L) / (BAUD * 16L) - 1) 

#define ASCII_DWUKROPEK 0x3A



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
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>

#include "sd_spi.h"
#include "fat16.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Pozwala wykonaæ skok do kodu po zakoñczeniu pracy bootloadera
static void (*jump_to_app)(void) = 0x0000;

// //Wysy³a bajt przez uart
// static void SendByte( const char Data )
// {
	// while( !( UCSRA & (1<<UDRE) ) );
	// UDR = Data;
// }

// //Odbiera bajt przez uart
// static uint8_t ReadByte(void)
// {
	// while ( !( UCSRA & (1<<RXC) ) );
	// return UDR;
// }

// static uint8_t ReadByteWait(void){
		// uint8_t Timeout=200;
		// do{
		// if ( UCSRA & 1 << RXC )
            // return UDR;
             // _delay_ms(10);
	// } while ( --Timeout );
	// return 0xFF;
// }


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
int main(void) __attribute__ ((section (".init9"),  used, noreturn )); 
int main(void)
{
	char ret;
	
	#ifdef UART_DEBUG	
		UARTInit();
	#endif

	SPI_init();
    
	ret = SD_init();
	if(ret) 
	{
        #ifdef UART_DEBUG
			UARTSendString("EXIT: SD err: ");
			itoa(ret,po_konwersji,10);
			UARTSendString(po_konwersji);
		#endif
		jump_to_app();
    }
	
    ret = fat16_init();
	if(ret) 
	{
        #ifdef UART_DEBUG
			UARTSendString("\r\nEXIT: FAT err: ");
			itoa(ret,po_konwersji,10);
			UARTSendString(po_konwersji);
        #endif
		jump_to_app();
    }

	ret = fat16_open_file(FILE_NAME,FILE_EXT);       
    if(ret) 
	{
        #ifdef UART_DEBUG
			UARTSendString("\r\nEXIT: File open err: ");
			itoa(ret,po_konwersji,10);
			UARTSendString(po_konwersji);
		#endif
        jump_to_app();
    }
	
	
	
	


	uint8_t rindex=0,bajtow_w_rekordzie=0, typ_rekordu=0, suma_kontrolna=0;
	uint8_t	suma_kontrolna_odczytana=0, bajt_danych=0, koniec_danych=0,index_slowa=0;
	uint16_t adres=0,pageAdress=0xFFFF,Byte_Address=0, slowo_danych=0;
	char bufor[8];
	boot_spm_busy_wait();
	while(fat16_state.file_left) 
	{
		ret = fat16_read_file(FAT16_BUFFER_SIZE);
		for(uint8_t i=0; i<ret; i++) //Jedziemy z naszym FAT16_BUFFER_SIZE buforkiem :)
        {
			if(!rindex && fat16_buffer[i]==ASCII_DWUKROPEK) //Mamy nowa linie
			{
				rindex++;
			}else if(rindex && rindex<3) //Do bufora dwa znaki dlugosci rekordu
			{
				bufor[rindex-1]=fat16_buffer[i];
				rindex++;
			}else if(rindex && rindex<7) //Do bufora 4 znakowy adres
			{
				if(rindex==3) //Konczymy string w buforze i zamieniamy na ilosc bajtow danych w linii
				{	
					bufor[rindex-1]=0;
					bajtow_w_rekordzie=strtol(bufor,NULL,16);
					//suma_kontrolna=bajtow_w_rekordzie; //Pierszy bajt do sumu kontrolnej
				}
				if(bajtow_w_rekordzie) //Skladamy 4 znakowy adres
				{
					bufor[rindex-3]=fat16_buffer[i]; //Zapelniamy bufor nowymi znakami
					koniec_danych=((bajtow_w_rekordzie*2)+9); //Do ktorego znaku w rekordzie siegaja dane ?
				}else
					koniec_danych=0;
				rindex++;
			}else if(rindex && bajtow_w_rekordzie && rindex < 9 ) //Do bufora dwuznakowy typ rekordu
			{
				if(rindex==7) //Konczymy string w buforze i zamienimy na 2 bajtowy adres
				{
					bufor[rindex-3]=0;
					adres=strtol(bufor,NULL,16);
					if(Byte_Address>0 && (pageAdress+Byte_Address)!=adres) //Mamy juz cos w buforze, a nowy adres rekordu nie jest kontynuacja adresowania w buforze
					{
						Byte_Address+=2;
						while(Byte_Address<SPM_PAGESIZE)
						{
							boot_page_fill( Byte_Address, 0x0000 );
							Byte_Address += 2;							
						}
						boot_page_erase( pageAdress ); //kasujemy stronê
						boot_spm_busy_wait();
						boot_page_write( pageAdress ); //zapisujemy strone nowymi danymi
						boot_spm_busy_wait();
						
						//Co tu dalej ? trzeba sie zastanowic
					}
					//To ponizej, tez nie jestem pewien, czy prawidlowo tutaj sie znajduje - do sprawdzenia.
					if(pageAdress==0xFFFF)
						pageAdress=adres; //Pierwszy adres powinien byc poczatkiem strony - póki co brak walidacji
					
					suma_kontrolna+=(adres&0x00FF); //Mlodszy bajt adresu dodajemy do sumy kontrolnej
					suma_kontrolna+=(adres>>8);		//Starszy bajt adresu dodajemy do sumy kontrolnej
				}
				bufor[rindex-7]=fat16_buffer[i]; //Zapelniamy bufor nowymi znakami
				rindex++;
			}else if(rindex && bajtow_w_rekordzie && rindex<koniec_danych) //Tu bedziemy czytac dane
			{
				if(rindex==9) //Konczymy string w buforze i zamienimy na typ rekordu
				{
					bufor[rindex-7]=0;
					typ_rekordu=strtol(bufor,NULL,16);
					suma_kontrolna+=typ_rekordu; //Typ rekordu dodajemy do sumy kontrolnej
					#ifdef UART_DEBUG
						UARTSendString("\n\rBajtow: ");
						itoa(bajtow_w_rekordzie,po_konwersji,10);
						UARTSendString(po_konwersji);
						UARTSendString("\n\rAdres: ");
						itoa(adres,po_konwersji,16);
						UARTSendString(po_konwersji);
						UARTSendString("\n\rTyp: ");
						itoa(typ_rekordu,po_konwersji,10);
						UARTSendString(po_konwersji);
						UARTSendString("\n\rDane: ");					
					#endif
				}					
				uint8_t indeks=(rindex+1)&0x01; //Indeks bedzie 0 i 1 na przemian, bo chcemy czytac jednobajtow_w_rekordziee wartosci hex(dwuznakowe)
				bufor[indeks]=fat16_buffer[i]; //Zapelniamy bufor nowymi znakami
				if(indeks && !typ_rekordu) //Drugi znak w buforze, zamieniamy na bajt danych, jesli ten rekord to rekord z danymi
				{
					bufor[indeks+1]=0;
					bajt_danych=strtol(bufor,NULL,16); //Mamy gotowy bajt danych
					if(index_slowa)
					{
						slowo_danych=bajt_danych<<8; //Starszy bajt do slowa
						index_slowa++;
					}else
					{
						index_slowa=0;
						slowo_danych+=bajt_danych; //Mlodszy bajt do slowa
						boot_page_fill( Byte_Address, slowo_danych );
						Byte_Address += 2;
						if(Byte_Address >= SPM_PAGESIZE) //Bufor strony gotowy
						{
							boot_page_erase( pageAdress ); //kasujemy stronê
							boot_spm_busy_wait();
							boot_page_write( pageAdress ); //zapisujemy strone nowymi danymi
							boot_spm_busy_wait();
							Byte_Address=0;
							pageAdress=adres+((rindex-9)>>1); // Ustalamy nowy adres strony danych (odczytany z ostatniego rekordu ihex plus juz wykorzystane dane z rekordu)
						}
						
					}
					suma_kontrolna+=bajt_danych; //Wszystkie bajty danych po kolei dodawane do sumy kontrolnej
					#ifdef UART_DEBUG
						itoa(rindex,po_konwersji,10);
						UARTSendString("(");
						UARTSendString(po_konwersji);
						UARTSendString(")0x");
						itoa(bajt_danych,po_konwersji,16);
						UARTSendString(po_konwersji);
						UARTSendString(",");
					#endif
					
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
					#ifdef UART_DEBUG
						itoa(suma_kontrolna,po_konwersji,16);
						UARTSendString("\n\rSuma wyliczona: ");
						UARTSendString(po_konwersji);
						UARTSendString("\n\rSuma odczytana: ");
						itoa(suma_kontrolna_odczytana,po_konwersji,16);
						UARTSendString(po_konwersji);
						UARTSendString("\n\r");
					#endif
					if(suma_kontrolna==suma_kontrolna_odczytana) //Mamy prawidlowy rekord
					{
						if(!typ_rekordu) //rekord z danymi
						{
							
						}
					}else
					{
						#ifdef UART_DEBUG
							UARTSendString("\n\rEXIT: Blad sumy kontrolnej, adres 0x");
							itoa(adres,po_konwersji,16);
							UARTSendString(po_konwersji);
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

	// //W³¹cz uart
	// UBRRL = BAUD_PRESCALE;
	// UBRRH = (BAUD_PRESCALE >> 8);
	// UCSRB |= (1<<RXEN)|(1<<TXEN); 	
	// _delay_ms(10);
	
	
	// // ^ dane:8 bitów, 1 bit parzystoœci

	// // Wysy³amy znak zachêty i czekamy na odpowiedz
	// SendByte( '?' );


	// //Sprawdzenie je¿eli nie ma odpowiedzi lub ró¿ni siê ona od 's' to bootloader skacze do programu

	
	
	// uint8_t test = ReadByteWait();
	// uint8_t low,high,data;
	// if( test == 's')
	// {
		// //Wysy³amy nag³ówek
		// SendByte( 0xA0 ); // Oznaczenie wersji bootloadera
		// SendByte( SIGN ); // Sygnatura procesora z Makefile
		// SendByte( boot_lock_fuse_bits_get( GET_LOW_FUSE_BITS  ) );  //Fusebity
		// SendByte( boot_lock_fuse_bits_get( GET_HIGH_FUSE_BITS ) );
		// SendByte( boot_lock_fuse_bits_get( GET_LOCK_BITS      ) );
		
		// uint8_t command;
		// //G³ówna pêtla
		// while ( 1 )
		// {
			// // Odczyt rozkazu 
			// command = ReadByte();
			
			// // === ZAPIS PAMIÊCI FLASH ===
			// if (command=='F')
			// {
				 // uint8_t crc = 0xFF;
				// // odczekanie do zakoñczenia dzia³añ na EEPROM i flash 
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
					
				// //Znak gotowoœci na dane
				// SendByte( '>' );
					
				// //Zape³niamy bufor strony
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
				// // Wys³anie sumy kontrolnej w celu weryfikacji
				// SendByte(crc);
				// // Je¿eli weryfikacja siê powiod³a to zapisujemy
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