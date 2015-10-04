#include <avr/io.h>
#ifndef __FAT16_H
#define __FAT16_H


#define GLOBAL_CONFIG "config.h"

#ifdef GLOBAL_CONFIG
	#include GLOBAL_CONFIG
#else //Uncoment this section for local settings
	// #define UART_DEBUG

	// #ifdef UART_DEBUG
		// #define DEBUG
	// #endif
#endif


#ifdef UART_DEBUG
	#include "uart.h"
#endif

#ifdef FAT_DEBUG
	//#include <stdlib.h>
	//#include <avr/io.h>
	#include "uart.h"
#endif

#define FAT16_BUFFER_SIZE 32

extern uint32_t sd_sector;
extern uint16_t sd_pos;
extern int8_t SD_read(uint32_t sector, uint16_t offset, volatile char * buffer, uint16_t len);


char fat16_buffer[FAT16_BUFFER_SIZE];


// Partition information, starts at offset 0x1BE
typedef struct {
    uint8_t first_byte;
    uint8_t start_chs[3];
    uint8_t partition_type;
    uint8_t end_chs[3];
    uint32_t start_sector;
    uint32_t length_sectors;
} __attribute((packed)) PartitionTable;

// Bytes omitted from the start of Fat16BootSectorFragment
#define FAT16_BOOT_OFFSET 11

// Partial FAT16 boot sector structure - non-essentials commented out
typedef struct {
    /*uint8_t jmp[3];
    char oem[8];*/
    uint16_t sector_size;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t number_of_fats;
    uint16_t root_dir_entries;
    uint16_t total_sectors_short; // if zero, later field is used
    uint8_t media_descriptor;
    uint16_t fat_size_sectors;
    /*uint16_t sectors_per_track;
    uint16_t number_of_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_long;
    
    uint8_t drive_number;
    uint8_t current_head;
    uint8_t boot_signature;
    uint32_t volume_id;
    char volume_label[11];
    char fs_type[8];
    char boot_code[448];
    uint16_t boot_sector_signature;*/
} __attribute((packed)) Fat16BootSectorFragment;

// FAT16 file entry
typedef struct {
    uint8_t filename[8];
    uint8_t ext[3];
    uint8_t attributes;
    uint8_t reserved[10];
    uint16_t modify_time;
    uint16_t modify_date;
    uint16_t starting_cluster;
    uint32_t file_size;
} __attribute((packed)) Fat16Entry;

// State data required by FAT16 library
typedef struct {
   uint32_t fat_start; // FAT start position
   uint32_t data_start; // data start position
   uint8_t sectors_per_cluster; // cluster size in sectors
   uint16_t cluster; // current cluster being read
   uint32_t cluster_left; // bytes left in current cluster
   uint32_t file_left; // bytes left in the file being read
} __attribute((packed)) Fat16State;

Fat16State fat16_state;


// Aliases for fat16_buffer in different formats
#define FAT16_part ((PartitionTable *)((void *)fat16_buffer))
#define FAT16_boot ((Fat16BootSectorFragment *)((void *)fat16_buffer))
#define FAT16_entry ((Fat16Entry *)((void *)fat16_buffer))
#define FAT16_ushort ((uint16_t *)((void *)fat16_buffer))

// The following functions need to be provided for the library

// Go to specified offset. Next fat16_read should continue from here
void fat16_seek(uint32_t offset);
// Read <bytes> to fat16_buffer, and return the amount of bytes read
char fat16_read(uint8_t bytes);

// The following functions are provided by the library

// Error codes for fat16_init()
#define FAT16_ERR_NO_PARTITION_FOUND 1
#define FAT16_ERR_INVALID_SECTOR_SIZE 2

// Initialize the FAT16 library. Nonzero return value indicates
// an error state. After init, the file stream is pointed to
// the beginning of root directory entry.
char fat16_init(void); // nonzero return values indicate error state

// Error codes for fat16_open_file()
#define FAT16_ERR_FILE_NOT_FOUND 1
#define FAT16_ERR_FILE_READ 2

// Open a given file. Assumes that the stream is currently at
// a directory entry and fat16_state.file_left contains the
// amount of file entries available. Both conditions are satisfied
// after fat16_init(), and also after opening a directory with this.
char fat16_open_file(char *filename, char *ext);

// Read <bytes> from file. Returns the bytes actually read, and
// 0 if end of file is reached. This method will automatically
// traverse all clusters of a file using the file allocation table
char fat16_read_file(char bytes);

#endif
