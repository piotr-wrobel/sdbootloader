// Obsługa FAT16 w projekcie SDBootloader bazuje na poradniku 
// Simple FAT and SD Tutorial (http://codeandlife.com/2012/04/02/simple-fat-and-sd-tutorial-part-1/)
// modyfikacje pvg, licencja GPL 3.0


#include "fat16.h"


// Initialize the library - locates the first FAT16 partition,
// loads the relevant part of its boot sector to calculate
// values needed for operation, and finally positions the
// file reading routines to the start of root directory entries
char fat16_init() 
{
    uint8_t i;
    uint32_t root_start;

    fat16_seek(0x1BE);
        
    for(i=0; i<4; i++) { 
        fat16_read(sizeof(PartitionTable));
#ifdef FAT_DEBUG
		uint8_t j;
		UARTSendString("Partycja nr [");
		UARTuitoa(i,po_konwersji);
		UARTSendString(po_konwersji);
		UARTSendString("] typ: ");
		UARTuitoa(FAT16_part->partition_type,po_konwersji);
		UARTSendString(po_konwersji);
		UARTSendString("\r\n RAW: ");
		for(j=0; j<sizeof(PartitionTable); j++)
		{
			UARTuitoa(fat16_buffer[j],po_konwersji);
			if(!(j%8))
				UARTSendString("\r\n");
			else
				UARTSendString(" ");
			UARTSendString(po_konwersji);
				
		}
		UARTSendString("\r\n");
			
#endif       
        if(FAT16_part->partition_type == 4 || 
           FAT16_part->partition_type == 6 ||
           FAT16_part->partition_type == 14)
            break;
    }
    
    if(i == 4) // none of the partitions were FAT16
        return FAT16_ERR_NO_PARTITION_FOUND;
    
    fat16_state.fat_start = 512 * FAT16_part->start_sector; // temporary
    
    fat16_seek(fat16_state.fat_start + FAT16_BOOT_OFFSET);
    fat16_read(sizeof(Fat16BootSectorFragment));
    
    if(FAT16_boot->sector_size != 512)
        return FAT16_ERR_INVALID_SECTOR_SIZE;
    
    fat16_state.fat_start += FAT16_boot->reserved_sectors * 512;
 
#ifdef FAT_DEBUG
    fat16_seek(fat16_state.fat_start);
    UARTSendString("FAT + reserved at: ");
	UARTuitoa(sd_sector>>16,po_konwersji);
	UARTSendString(po_konwersji);
	UARTuitoa(sd_sector&0xFFFF,po_konwersji);
	UARTSendString(po_konwersji);	
	UARTSendString(" offset: ");
	UARTuitoa(sd_pos,po_konwersji);
	UARTSendString(po_konwersji);
	UARTSendString("\r\n");
	UARTuitoa(FAT16_boot->fat_size_sectors,po_konwersji);
	UARTSendString("FAT size sectors: ");
	UARTSendString(po_konwersji);
	UARTuitoa(FAT16_boot->number_of_fats,po_konwersji);
	UARTSendString(" Numbers of FAT'S: ");
	UARTSendString(po_konwersji);
	UARTSendString("\r\n");
#endif 
    root_start = fat16_state.fat_start + ((uint32_t)(FAT16_boot->fat_size_sectors) * FAT16_boot->number_of_fats * 512);
    
    fat16_state.data_start = root_start + ((uint32_t)(FAT16_boot->root_dir_entries) * 
        sizeof(Fat16Entry));
        
    fat16_state.sectors_per_cluster = FAT16_boot->sectors_per_cluster;
    
    // Prepare for fat16_open_file(), cluster is not needed
    fat16_state.file_left = FAT16_boot->root_dir_entries * sizeof(Fat16Entry);
    fat16_state.cluster_left = 0xFFFFFFFF; // avoid FAT lookup with root dir

           
    fat16_seek(root_start);
#ifdef FAT_DEBUG
    UARTSendString("Root start at: ");
	UARTuitoa(sd_sector>>16,po_konwersji);
	UARTSendString(po_konwersji);
	UARTuitoa(sd_sector&0xFFFF,po_konwersji);
	UARTSendString(po_konwersji);	
	UARTSendString(" offset: ");
	UARTuitoa(sd_pos,po_konwersji);
	UARTSendString(po_konwersji);
	UARTSendString("\r\n");
#endif	

    return 0;
}

// Assumes we are at the beginning of a directory and fat16_state.file_left
// is set to amount of file entries. Reads on until a given file is found,
// or no more file entries are available.
char fat16_open_file(char *filename, char *ext)
{  
    uint8_t i, bytes;
    
#ifdef FAT_DEBUG
    UARTSendString("Trying to open file [");
	UARTSendString(filename);
	UARTSendString(".");
	UARTSendString(ext);
	UARTSendString("]\r\n");
#endif

    do {
        bytes = fat16_read_file(sizeof(Fat16Entry));
        
        if(bytes < sizeof(Fat16Entry))
            return FAT16_ERR_FILE_READ;
		
#ifdef FAT_DEBUG
        char plik[13];
		if(FAT16_entry->filename[0])
        {    
			for(i=0; i<8; i++)
				plik[i]=FAT16_entry->filename[i];
			plik[8]='.';
			for(i=0; i<3; i++)
				plik[i+9]=FAT16_entry->ext[i];			
			UARTSendString("Found file: ");
			UARTSendString(plik);
			UARTSendString("\r\n");
		}
#endif

        for(i=0; i<8; i++) // we don't have memcmp on a MCU...
            if(FAT16_entry->filename[i] != filename[i])
                break;        
        if(i < 8) // not the filename we are looking for
            continue;
		
        for(i=0; i<3; i++) // we don't have memcmp on a MCU...
            if(FAT16_entry->ext[i] != ext[i])
                break;
        if(i < 3) // not the extension we are looking for
            continue;
            

        // Initialize reading variables
        fat16_state.cluster = FAT16_entry->starting_cluster;
        fat16_state.cluster_left = fat16_state.sectors_per_cluster * 512;
        
        if(FAT16_entry->filename[0] == 0x2E || 
           FAT16_entry->attributes & 0x10) { // directory
            // set file size so large that the file entries
            // are not limited by it, but by the sectors used
            fat16_state.file_left = 0xFFFFFFFF;
        } else {
            fat16_state.file_left = FAT16_entry->file_size;
        }
        
        // Go to first cluster
        fat16_seek(fat16_state.data_start + ((uint32_t)(fat16_state.cluster-2) * 
            fat16_state.sectors_per_cluster * 512));
        return 0;
    } while(fat16_state.file_left > 0);
    
    return FAT16_ERR_FILE_NOT_FOUND;
}

char fat16_read_file(char bytes) // returns the bytes read
{ 
    if(fat16_state.file_left == 0)
        return 0;
    
    if(fat16_state.cluster_left == 0) {
        fat16_seek(fat16_state.fat_start + (uint32_t)(fat16_state.cluster)*2);
        fat16_read(2);
        
        fat16_state.cluster = FAT16_ushort[0];
        fat16_state.cluster_left = (uint32_t)(fat16_state.sectors_per_cluster) * 512;
        
        if(fat16_state.cluster == 0xFFFF) { // end of cluster chain
            fat16_state.file_left = 0;
            return 0;
        }
            
        // Go to first cluster
        fat16_seek(fat16_state.data_start + ((uint32_t)(fat16_state.cluster-2) * 
            fat16_state.sectors_per_cluster * 512));
 
    }
    
    if(bytes > fat16_state.file_left)
        bytes = fat16_state.file_left;
    if(bytes > fat16_state.cluster_left)
        bytes = fat16_state.cluster_left;
        
    bytes = fat16_read(bytes);
    
    fat16_state.file_left -= bytes;
    fat16_state.cluster_left -= bytes;

    
    return bytes;
}

void fat16_seek(uint32_t offset) 
{
    sd_sector = offset >> 9;
    sd_pos = offset & 511;        
}

char fat16_read(uint8_t bytes) 
{
    SD_read(sd_sector, sd_pos, fat16_buffer, bytes);
    sd_pos+=(uint16_t)bytes;
    
    if(sd_pos == 512) {
        sd_pos = 0;
        sd_sector++;
    }
    
    return bytes;
}

