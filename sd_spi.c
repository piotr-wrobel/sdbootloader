#include "sd_spi.h"

uint8_t SPI_write(uint8_t ch)
{
	#ifdef SD_DEBUG
		UARTuitoa(ch,po_konwersji);
		UARTSendString("S: ");
		UARTSendString(po_konwersji);
	#endif
		SPDR = ch;
		while(!(SPSR & (1<<SPIF)));	
		ch=SPDR;
	#ifdef SD_DEBUG
		UARTSendString(" R: ");
		UARTuitoa(SPDR,po_konwersji);
		UARTSendString(po_konwersji);
		UARTSendString("\r\n");
	#endif
		return ch;
}

static void SD_readR3R7(uint8_t * db)
{
	uint8_t i;
	for (i=0; i<4; i++) db[i] = SPI_read(); //Skip 4 bytes of response
	SPI_read();//Skip CRC7	
}

char SD_init(void) 
{
    uint8_t i,response,cmd,sd_version;
	uint8_t db[16];

    CS_DISABLE();
	for(i=0; i<10; i++) SPI_read();// idle for 1 bytes / 80 clocks
	CS_ENABLE();

	
    response = SD_command(SD_GO_IDLE, 0x00000000, SKIPCRC7);
    if(response!=1)
	{CS_DISABLE(); return 1;}
                
	response = SD_command(SD_SEND_IF_COND, 0x000001AA, NOSKIPCRC7);    // Check card voltage (type) for SD 1.0 or SD 2.0
	SD_readR3R7(db);
	if ( (response == 0xFF) || (response == 0x05))
		sd_version=1;
	else
		sd_version=2;
	
	cmd = SD_SEND_APP_OP;                        //send INIT or SEND_APP_OP repeatedly until receive a 0
	i=0;
	while (i++<255) 
	{
		if(cmd==SD_SEND_APP_OP)
		{
			response = SD_command(SD_APP_CMD, 0x00000000, SKIPCRC7);   // will still be in idle mode (0x01) after this
			if(sd_version==2)
				response = SD_command(cmd, 0x40000000, SKIPCRC7);
			else
				response = SD_command(cmd, 0x00000000, SKIPCRC7);
		} else
			response = SD_command(cmd, 0, SKIPCRC7);
		if ( (response &0x0F) == 0x05 || response == 0xFF ) // ACMD41 not recognized, use CMD1
			cmd = SD_SEND_OP_COND;
		
		_delay_ms(3);  //For some slow cards :)
		if (!response) 
			break;
	}
	if (response != 0)
	{CS_DISABLE(); return 2;}                          // failed to reset.

	if(sd_version==2)
	{
		response = SD_command(SD_READ_OCR,0x00000000,NOSKIPCRC7);            // 7. Check for capacity of the card
		if (response > 1)
		{CS_DISABLE(); return 3;}                          // error, bad thing.
		SD_readR3R7(db);
		if ( ((db[0] & 0x40) == 0x40) && (db[0] != 0xFF) )  // check CCS bit (bit 30), PoweredUp (bit 31) set if ready.
		{CS_DISABLE(); return 4;} // card is high capacity, not working here
	}
    
	//SET CRC to OFF
	SD_command(SD_CRC_ON_OFF, CRC_OFF,SKIPCRC7);
    // SET_BLOCKLEN to 512
    SD_command(SD_SET_BLOCK_LEN, SD_BLOCK_LEN,SKIPCRC7);
    sd_sector = sd_pos = 0;
	
	// Teraz juz pelna predkosc 20 MHz/4 = 5MHz
	SPCR = (1<<SPE) | (1<<MSTR); //fsck/4
	//SPSR = (1<<SPI2X);//dodatkowo fsck/2
    CS_DISABLE(); return 0;
}


// TODO: This function will not exit gracefully if SD card does not do what it should
int8_t SD_read(uint32_t sector, uint16_t offset, volatile char * buffer, uint16_t len) 
{

	unsigned char response;
	uint16_t i;

	response = SD_command(SD_READ_SINGLE_BLOCK, sector<<9,NOSKIPCRC7);
	if(response!=0x00)
		return response;
  
    CS_ENABLE();

    for(i=0; i<0xfffe && SPI_read() != 0xFE; i++); // wait for data start
    if(i==0xffff) 
	{
		CS_DISABLE();
		return 2;
	}
#ifdef SD_DEBUG
    UARTSendString("Skip section start\r\n");
#endif
    for(i=0; i<offset; i++) // "skip" bytes
        SPI_read();
#ifdef SD_DEBUG
    UARTSendString("Read section start\r\n");
#endif        
    for(i=0; i<len; i++) // read len bytes
        buffer[i] = SPI_read();
 #ifdef SD_DEBUG
    UARTSendString("Skip after section start\r\n");
#endif       
    for(i+=offset; i<512; i++) // "skip" again
        SPI_read();
        
    // skip checksum
    SPI_read();
    SPI_read();
	SPI_read();
    CS_DISABLE();
	return 0;    
}

void SPI_init(void)
{
	CS_DDR |= CS; // SD card circuit select as output
	DDRB |= MOSI | SCK; // MOSI and SCK as outputs
	PORTB |= MISO; // pullup in MISO, might not be needed
	
	// Enable SPI, master, set clock rate fck/128
	SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR0) | (1<<SPR1); //fck/128 //Na poczatek 20 000 KHz/128= 156KHz
}


uint8_t SD_command(uint8_t cmd, uint32_t arg, uint8_t read_and_skip_crc7)
{
	uint8_t i, buffer, crc=0x95; //buffer[8];
	if (cmd == SD_SEND_IF_COND)
		crc = 0x86;	// Special CRC for SD_SEND_IF_COND(0x1AA)
	CS_ENABLE();
	SPI_write(cmd | 0x40);
	SPI_write(arg>>24);
	SPI_write(arg>>16);
	SPI_write(arg>>8);
	SPI_write(arg);
	SPI_write(crc);

		
	for(i=0; i<9; i++) // Do 8 bajtow pomijamy
	{
		if((buffer = SPI_read())!=0xFF)
		{		
			if(read_and_skip_crc7)
				SPI_read();
			return buffer;
		}
	}
	CS_DISABLE();
	return 0xFF;
}
