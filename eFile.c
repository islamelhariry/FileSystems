// eFile.c
// Runs on either TM4C123 or MSP432
// High-level implementation of the file system implementation.
// Daniel and Jonathan Valvano
// August 29, 2016
#include <stdint.h>
#include "eDisk.h"
#include "eFile.h"

uint8_t Buff[512]; // temporary buffer used during file I/O
uint8_t Directory[256], FAT[256];
int32_t bDirectoryLoaded =0; // 0 means disk on ROM is complete, 1 means RAM version active

// Return the larger of two integers.
int16_t max(int16_t a, int16_t b){
  if(a > b){
    return a;
  }
  return b;
}
//*****MountDirectory******
// if directory and FAT are not loaded in RAM,
// bring it into RAM from disk
void MountDirectory(void){ 
// if bDirectoryLoaded is 0, 
//    read disk sector 255 and populate Directory and FAT
//    set bDirectoryLoaded=1
// if bDirectoryLoaded is 1, simply return
// **write this function**
	if(bDirectoryLoaded == 1)
		return;
	else{
		eDisk_ReadSector(Buff,255);
		for(uint8_t i=0;i<255;i++){
			Directory[i]=Buff[i];
			FAT[i]=Buff[i+256];
		}
		bDirectoryLoaded=1;
	}
}

// Return the index of the last sector in the file
// associated with a given starting sector.
// Note: This function will loop forever without returning
// if the file has no end (i.e. the FAT is corrupted).
uint8_t lastsector(uint8_t start){
// **write this function**
	uint8_t m;
  while(start !=255)	{
		m = FAT[start];
		if(m == 255)
			return start;
		else
			start = m;
	}
	if(start == 255)
		return 255;
  
	return 0; 
}

// Return the index of the first free sector.
// Note: This function will loop forever without returning
// if a file has no end or if (Directory[255] != 255)
// (i.e. the FAT is corrupted).
uint8_t findfreesector(void){
// **write this function**
  int8_t fs = -1;
	uint8_t i = 0;
	uint8_t ls = lastsector(Directory[i]);
	while(ls != 255)	{
		fs = max(fs,ls);
		i = i + 1;
		ls = lastsector(Directory[i]);
	}
	if(ls == 255)
		return fs + 1;
  return 0; 
}

// Append a sector index 'n' at the end of file 'num'.
// This helper function is part of OS_File_Append(), which
// should have already verified that there is free space,
// so it always returns 0 (successful).
// Note: This function will loop forever without returning
// if the file has no end (i.e. the FAT is corrupted).
uint8_t appendfat(uint8_t num, uint8_t n){
// **write this function**
  uint8_t i = Directory[num];
	uint8_t m;
	while(i!=255){
		m = FAT[i];
		if(m == 255)	{
			FAT[i] = n;
			return 0;		
		}
		else
			i = m;
	}
	if(i == 255)	{
		Directory[num] =n;
		return 0;
	}
	return 0;		
}

//********OS_File_New*************
// Returns a file number of a new file for writing
// Inputs: none
// Outputs: number of a new file
// Errors: return 255 on failure or disk full
uint8_t OS_File_New(void){
// **write this function** 
	if (bDirectoryLoaded == 0){
		MountDirectory();
		bDirectoryLoaded =1;
	}
	uint8_t i=0;
	while(Directory[i] != 255){
		i=i+1;
		if(i==255)
			return 255;
	}
	return i;
}

//********OS_File_Size*************
// Check the size of this file
// Inputs:  num, 8-bit file number, 0 to 254
// Outputs: 0 if empty, otherwise the number of sectors
// Errors:  none
uint8_t OS_File_Size(uint8_t num){
// **write this function**
  if(bDirectoryLoaded==0)	{
		MountDirectory();
		bDirectoryLoaded =1;
	}
	
	uint8_t count = 0;
	uint8_t firstSector = Directory[num];
	uint8_t nextSector = FAT[firstSector];
	if(firstSector == 255)
		return count;
	else{
		while(nextSector != 255)
		{
			nextSector = FAT[nextSector];
			count++;
		}
	}	
  return count+1;
}

//********OS_File_Append*************
// Save 512 bytes into the file
// Inputs:  num, 8-bit file number, 0 to 254
//          buf, pointer to 512 bytes of data
// Outputs: 0 if successful
// Errors:  255 on failure or disk full
uint8_t OS_File_Append(uint8_t num, uint8_t buf[512]){
// **write this function**
  if(bDirectoryLoaded==0)	{
		MountDirectory();
		bDirectoryLoaded =1;
	}
	uint8_t n = findfreesector();
	if(n!=255)	{
		eDisk_WriteSector(buf,n);
		appendfat(num, n);
		OS_File_Flush();
		return 0; 
	}
	else
		return 255;
	
}

//********OS_File_Read*************
// Read 512 bytes from the file
// Inputs:  num, 8-bit file number, 0 to 254
//          location, logical address, 0 to 254
//          buf, pointer to 512 empty spaces in RAM
// Outputs: 0 if successful
// Errors:  255 on failure because no data
uint8_t OS_File_Read(uint8_t num, uint8_t location,
                     uint8_t buf[512]){
// **write this function**											 
	if(bDirectoryLoaded==0)	{
		MountDirectory();
		bDirectoryLoaded =1;
	}		
	
	uint8_t sector=Directory[num];
	uint8_t fsize = OS_File_Size(num);
	if(location > (fsize-1))
		return 255;
	else{
		uint8_t i=0;
		while(i<location && sector!=255)	{
			sector = FAT[sector];
			i++;
		}
	}							 
	if(eDisk_ReadSector(buf,sector) == RES_OK)
		return 0; 
	else
		return 255;
}

//********OS_File_Flush*************
// Update working buffers onto the disk
// Power can be removed after calling flush
// Inputs:  none
// Outputs: 0 if success
// Errors:  255 on disk write failure
uint8_t OS_File_Flush(void){
// **write this function**
	uint8_t i;
	if(bDirectoryLoaded==1)	{
		for(i=0; i<255; i++)	{
			Buff[i]=Directory[i];
			Buff[256+i]=FAT[i];
		}
		if (eDisk_WriteSector(Buff, 255) != RES_OK) 
			return 255;	// Error on writing data to the Disk
		else
			bDirectoryLoaded = 0;
	}
	return 0; 
}

//********OS_File_Format*************
// Erase all files and all data
// Inputs:  none
// Outputs: 0 if success
// Errors:  255 on disk write failure
uint8_t OS_File_Format(void){
// call eDiskFormat
// clear bDirectoryLoaded to zero
// **write this function**
	if(eDisk_Format()!=RES_OK)
		return 255;
	else{
		bDirectoryLoaded = 0;
		return 0; 
		}
}
