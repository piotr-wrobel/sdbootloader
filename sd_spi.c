#include "sd_spi.h"


char SD_init(void) 
{
    char i;
        
    // ]r:10
    CS_DISABLE();
    for(i=0; i<10; i++) // idle for 1 bytes / 80 clocks
        SPI_write(0xFF);
                
    // [0x40 0x00 0x00 0x00 0x00 0x95 r:8] until we get "1"
    for(i=0; i<10 && SD_command(0x40, 0x00000000, 0x95, 255) != 1; i++)
        _delay_ms(100);
                        
    if(i == 10) // card did not respond to initialization
        return 1;
                
    // CMD1 until card comes out of idle, but maximum of 10 times
    for(i=0; i<10 && SD_command(0x41, 0x00000000, 0xFF, 255) != 0; i++)
        _delay_ms(100);

    if(i == 10) // card did not come out of idle
        return 2;
                
    // SET_BLOCKLEN to 512
    if(SD_command(0x50, 0x00000200, 0xFF, 255)==0xFF)
		return 3;
	_delay_ms(100);
        
    sd_sector = sd_pos = 0;
        
    return 0;
}

// TODO: This function will not exit gracefully if SD card does not do what it should
int8_t SD_read(uint32_t sector, uint16_t offset, volatile char * buffer, uint16_t len) 
{
    uint16_t i;
    
    CS_ENABLE();
    SPI_write(0x51);
    SPI_write(sector>>15); // sector*512 >> 24
    SPI_write(sector>>7);  // sector*512 >> 16
    SPI_write(sector<<1);  // sector*512 >> 8
    SPI_write(0);          // sector*512
    SPI_write(0xFF);
    
    for(i=0; i<10 && SPI_write(0xFF) != 0x00; i++) {} // wait for 0
    if(i==10) return 1;
    for(i=0; i<10 && SPI_write(0xFF) != 0xFE; i++) {} // wait for data start
    if(i==10) return 2;
#ifdef DEBUG
    UARTSendString("Skip section start\r\n");
#endif
    for(i=0; i<offset; i++) // "skip" bytes
        SPI_write(0xFF);
#ifdef DEBUG
    UARTSendString("Read section start\r\n");
#endif        
    for(i=0; i<len; i++) // read len bytes
        buffer[i] = SPI_write(0xFF);
 #ifdef DEBUG
    UARTSendString("Skip after section start\r\n");
#endif       
    for(i+=offset; i<512; i++) // "skip" again
        SPI_write(0xFF);
        
    // skip checksum
    SPI_write(0xFF);
    SPI_write(0xFF);
	

    CS_DISABLE();
	return 0;    
}

void SPI_init(void)
{
	CS_DDR |= CS; // SD card circuit select as output
	DDRB |= MOSI + SCK; // MOSI and SCK as outputs
	PORTB |= MISO; // pullup in MISO, might not be needed
	
	// Enable SPI, master, set clock rate fck/128
	SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR0) | (1<<SPR1);
	//SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR1); //fsck/64
}

uint8_t SPI_write(uint8_t ch)
{
	#ifdef DEBUG
		itoa(ch,po_konwersji,16);
		UARTSendString("S: ");
		UARTSendString(po_konwersji);
	#endif
		_delay_us(200);
		SPDR = ch;
		while(!(SPSR & (1<<SPIF))) {}	
	#ifdef DEBUG
		UARTSendString(" R: ");
		itoa(SPDR,po_konwersji,16);
		UARTSendString(po_konwersji);
		UARTSendString("\r\n");
	#endif
		//_delay_us(100);
		return SPDR;
}

uint8_t SD_command(uint8_t cmd, uint32_t arg, uint8_t crc, uint8_t read)
{
	uint8_t i, buffer; //buffer[8];
	CS_ENABLE();
	SPI_write(cmd);
	SPI_write(arg>>24);
	SPI_write(arg>>16);
	SPI_write(arg>>8);
	SPI_write(arg);
	SPI_write(crc);
		
	for(i=0; i<read; i++)
	{
		if((buffer = SPI_write(0xFF)) < 0xFF)
		{		
			SPI_write(0xFF);
			CS_DISABLE();
			return buffer;
		}
	}
	CS_DISABLE();
	return 0xFF;
}
