/*
 * Superblock initialization, formatting of disk, file system params.
 * Also acts as interface to copy form cache to workspace.
 */

#include "blocks.h"
#include <string.h>


struct SuperBlock disk_superblock;


// Writes block from working area to disk cache
int Write_Block(int blockNo, char* data) {
	char* block;
	int rc = 0;
	// Get block into cache (Write back)
	rc = Get_Into_Cache(blockNo, &block);
	if (rc)
	{
		Print("blocks.c/Write_Block: Could not get block into cache.\n");
		return rc;
	}
	// Copy the data
	memcpy((void*)block, data, disk_superblock.blockSizeInBytes);
	// Set dirty bit
	rc = Set_Dirty(blockNo);
	if(rc) { 
		Print("blocks.c/Write_Block: BUG::Could not set block as dirty after writing.\n");
		return rc;
	}
	// Release cache lock
	rc = Unfix_From_Cache(blockNo);
	if(rc)
	{
		Print("blocks.c/Write_Block: BUG::Could not unfix from cache.\n");
		return rc;
	}
	return 0;
}

// Reads block from disk cache to working area
int Read_Block(int blockNo, char* dataDest) {
	// Asssume dataDest is correct size, ie one block
	char* block;
	// Get into cache
	int rc = Get_Into_Cache(blockNo, &block);
	if (rc)
	{
		Print("blocks.c/Read_Block: Could not get block into cache.\n");
		return rc;
	}
	// Do a memory copy
	memcpy((void*)dataDest, (void*)block, disk_superblock.blockSizeInBytes);
	// Release cache lock
	rc = Unfix_From_Cache(blockNo);
	
	if(rc)
	{
		Print("blocks.c/Read_Block: BUG::Could not unfix from cache.\n");
		return rc;
	}
	return 0;
}

// Added a syscall for this
// Formats the sim disk
int Format_Disk() {
	Print("Formatting the disk...\n");
	// Assumes sim_disk is initialized
	int rc;
	// Format and write superblock onto disk
	rc = Format_Super_Block();
	if (rc){
		Print("blocks.c/Format_Disk: Could not format superblock\n");
		return rc;
	}
	// Format the free list bitmap
	rc = Init_Free_List();
	if (rc)
	{
		Print("blocks.c/Format_Disk: Free list initialization failed\n");
		return rc;
	}
	// Format the inode bitmap
	rc = Format_Inode_Blocks();
	if(rc)
	{
		Print("blocks.c/Format_Disk: Could not format Inode structures\n");
		return rc;
	}
	Print("Disk formatting done. \n");
	return 0;
}

// Initializes the file system
int Init_File_System() {
	Print("Initializing file system...\n");
	int rc = 0;
	// Initialize simulated disk
	rc = Init_Sim_Disk();
	if (rc)
	{
		Print("blocks.c/Init_File_System: Could not initialize the sim disk");
		return rc;
	}
	// Read the superblock into memory
	rc = Read_Super_Block();
	if (rc) {
		Print("blocks.c/Init_File_System: Could not read superblock from disk\n");
		return rc;
	}
	// Initialize disk cache
	rc = Init_Disk_Cache();
	if (rc) {
		Print("blocks.c/Init_File_System: Could not initializa the disk cache\n");
		return rc;
	}
	// Initialize open file tables from processes
	// rc = Init_OFT();
	// if(rc) return rc;
	rc = Init_Inode_Manager();
	if(rc)
	{
		Print("blocks.c/Init_File_System: Could not initialize the inode manager\n");
		return rc;
	}
	Print("File system initialized.\n");
	return 0;
}

int Shut_Down_File_System()
{
	Print("Shutting down file system...\n");
	int rc = Shut_Down_Inode_Manager();
	if(rc)
	{
		Print("blocks.c/Shut_Down_File_System: Could not shut down the inode manager\n");
	}
	rc = Shut_Down_Disk_Cache() | rc;
	if(rc)
	{
		Print("blocks.c/Shut_Down_File_System: Could not shut down the disk\n");
	}
	rc = Shut_Down_Sim_Disk() | rc;
	if(rc)
	{
		Print("blocks.c/Shut_Down_File_System: Could not shut down sim disk\n");
	}
	Print("Finished shutting down file system.\n");
	return rc;
}

// Initializes the superblock using various parameters, and writes it to disk
int Format_Super_Block() {
	Print("Formatting super block...\n");
	
	disk_superblock.blockSizeInBytes = disk_hw_data.bytes_per_block;

	disk_superblock.totBlocks = disk_hw_data.blocks_per_track 
					* disk_hw_data.tracks_per_cylinder 
					* disk_hw_data.tot_cylinders;

	disk_superblock.diskCacheSize = DISK_CACHE_SIZE;

	disk_superblock.firstFreeListBitmapBlock = 1;

	disk_superblock.numFreeListBitmapBlocks = 
		((disk_superblock.totBlocks) / (disk_superblock.blockSizeInBytes * 8)) + 1;

	disk_superblock.firstInodeBitmapBlock =
		 disk_superblock.firstFreeListBitmapBlock + disk_superblock.numFreeListBitmapBlocks;

	disk_superblock.numInodeBlocks = 20;

	disk_superblock.numInodeBitmapBlocks = 
		(disk_superblock.numInodeBlocks * (disk_superblock.blockSizeInBytes 
		/ sizeof(35))) / (8 * disk_superblock.blockSizeInBytes) + 1;
;
	disk_superblock.firstInodeBlock = 
		disk_superblock.firstInodeBitmapBlock + disk_superblock.numInodeBitmapBlocks;

	disk_superblock.rootDirectoryBlock = disk_superblock.firstInodeBlock + disk_superblock.numInodeBlocks;	

	disk_superblock.numAllocatedBlocks = 
		disk_superblock.rootDirectoryBlock + 1; // 1 for root directory

	// Write the superblock to disk after formating
	int rc = Write_Super_Block();
	if(rc){
		Print("blocks.c/Format_Super_Block: Could not write superblock to disk\n");
		return rc;
	}
	Print("Done formatting superblock\n");
	return 0;
}



// Reads superblock into the global struct
int Read_Super_Block() {

	char block[BLOCK_SIZE];
	int rc = Read_From_Disk(block, SUPERBLOCK_NO, 1);
	if (rc)
	{
		Print("blocks.c/Read_Super_Block: Could not read from disk\n");
		return rc; 
	}
	memcpy((void*) &disk_superblock, (void*) block, sizeof(SuperBlock));
	return 0; 
}

// Write superblock to disk
int Write_Super_Block() {
	char block[BLOCK_SIZE];
	int i;
	for(i = 0; i<BLOCK_SIZE; ++i)
	{
		block[i] = 0;
	}
	memcpy((void*) block, (void*) &disk_superblock, sizeof(SuperBlock));
	int rc = Write_To_Disk(block, SUPERBLOCK_NO, 1);
	if(rc)
	{
		Print("blocks.c/Write_Super_Block: Could not write to disk\n");
		return rc;
	}
	return 0;
}

// Initializes the freelist bitmap with zeros
int Init_Free_List() {
	Print("Initializing free list...\n");
	char buf[BLOCK_SIZE];
	char bitmapArr[] = {0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF}; 
	int i, j, rc = 0;
	// Make a buffer with all zeros
	for(i = 0; i < BLOCK_SIZE; ++i)
	{
		buf[i] = 0;
	}
	
	// Write the zero buffer to all the freelist bitmap blocks except first
	for(i = 1; i < disk_superblock.numFreeListBitmapBlocks; ++i)
	{
		rc = rc | Write_To_Disk(buf, i + disk_superblock.firstFreeListBitmapBlock, 1);
	}
	if(rc)
	{
		Print("blocks.c/Init_Free_List: Could not write all free list blocks to disk\n");
		return rc;
	}
	
	// Prepare the buffer with 1s in place of the metadata blocks
	
	fflush(stdout);
	for(i = 8, j = 0; i < disk_superblock.numAllocatedBlocks; i += 8, ++j)
	{
		buf[j] = 0xFF;
	}
	
	int numOnes = 8 - (i - disk_superblock.numAllocatedBlocks);
	buf[j] = bitmapArr[numOnes];
	// Write these ones to disk
	// Assume all metadata block bits fit in block 1.
	rc = Write_To_Disk(buf, disk_superblock.firstFreeListBitmapBlock, 1);
	if(rc)
	{
		Print("blocks.c/Init_Free_List: Could not write all free list blocks to disk\n");
		return rc;
	}
	Print("Free list initialization completed\n");
	return 0;
}


// Initialize the inode bitmap, and set the root directory inode
int Format_Inode_Blocks()
{
	Print("Formatting inode blocks...\n");
	char buf[BLOCK_SIZE];
	int i, rc = 0;
	for(i = 0; i < BLOCK_SIZE; ++i)
	{
		buf[i] = 0;
	}
	for(i = 1; i < disk_superblock.numInodeBitmapBlocks; ++i)
	{
		rc = rc | Write_To_Disk(buf, i + disk_superblock.firstInodeBitmapBlock, 1);
	}
	if(rc)
	{
		Print("blocks.c/Format_Inode_Blocks: Could not write some inode blocks to disk\n");
		return rc;
	}
	
	buf[0] = 0x80;
	rc = Write_To_Disk(buf, disk_superblock.firstInodeBitmapBlock, 1);
	if(rc)
	{
		Print("blocks.c/Format_Inode_Blocks: Could not write some inode blocks to disk\n");
		return rc;
	}
	Inode inode;
	strcpy(inode.meta_data.filename, "root");
	inode.meta_data.group_id = 1; // all
	inode.meta_data.owner_id = 1; // root
	inode.meta_data.permissions = 0177; // All can read, group and owner can write/exec
	inode.entries[0] = disk_superblock.rootDirectoryBlock;
	buf[0] = 0x00;
	memcpy(buf, &inode, sizeof(Inode));
	
	rc = Write_To_Disk(buf, disk_superblock.firstInodeBlock, 1);
	if(rc)
	{
		Print("blocks.c/Format_Inode_Blocks: Could not write root inode to disk\n");
		return rc;
	}
	//~ DirHeader root_dir_header;
	//~ root_dir_header.numEntries = 0;
	//~ root_dir_header.parentInode = -1;
	//~ for(i = 0; i < BLOCK_SIZE; ++i)
	//~ {
		//~ buf[i] = 0;
	//~ };
	//~ memcpy(buf, &root_dir_header, sizeof(DirHeader));
	//~ rc = Write_To_Disk(buf, disk_superblock.rootDirectoryBlock, 1);
	//~ if(rc)
	//~ {
		//~ Print("blocks.c/Format_Inode_Blocks: Could not write some inode blocks to disk");
		//~ return rc;
	//~ }
	
	
	Print("Finished formatting Inode blocks.\n");
	return 0;
}


// Traverses the free blocks bitmap and finds a free block 
int Allocate_Block(int* freeBlock) {
	
	char* buf;
	int i, j, rc = 0;
	int flag = 0;
	
	// Loop to go over all bitmap blocks
	for(i = 0; i < disk_superblock.numFreeListBitmapBlocks && flag == 0; ++i)
	{
	fflush(stdout);
		rc = Get_Into_Cache(i + disk_superblock.firstFreeListBitmapBlock, &buf);
	fflush(stdout);
		if(rc) return rc; // Get into cache error. Fatal
		// Loop to go over all chars in block
		for(j = 0; j < disk_superblock.blockSizeInBytes && flag == 0; ++j)
		{
			if(buf[j] != (char)0xFF) // If found a 0 bit
			{
				
				flag = 1;
				int k;
				// Loop to go over bits in a char
				for(k = 0; k < 8; ++k)
				{
					fflush(stdout);
					if((buf[j] | (char)0x1 << (7 - k))  != buf[j])
					{
						buf[j] = buf[j] | (char)0x1 << (7 - k);
						int freeBlockNum = (disk_superblock.blockSizeInBytes * i + j) * 8 + k;
						if(freeBlockNum > disk_superblock.totBlocks)
						{
							rc = -1;
						}
						*freeBlock = freeBlockNum;
						if(!rc)
						{
							rc = Set_Dirty(i + disk_superblock.firstFreeListBitmapBlock);
						}
						break;
					}
				}
			}
			if(flag) break;
		}
		
		rc = Unfix_From_Cache(i + disk_superblock.firstFreeListBitmapBlock) | rc;
		if(flag && !rc) rc = Flush_Cache_Block(i + disk_superblock.firstFreeListBitmapBlock);	
	}
	return rc;
}

/* Frees the given block number by setting the corresponding bit in the bitmap to 0 */
int Free_Block(int blockNo) {

	if(blockNo > disk_superblock.totBlocks) return -1;
	// Calculate the block number for the corresponding bit
	int freeListBlockNo = disk_superblock.firstFreeListBitmapBlock + (blockNo / (disk_superblock.blockSizeInBytes * 8));
	// Calculate char number for corresponding bit
	int charNumber = (blockNo % disk_superblock.blockSizeInBytes) / 8;
	// Calculate bit number for corresponding bit
	int bitNumber = blockNo % 8;
	
	char *buf;
	int rc = Get_Into_Cache(freeListBlockNo, &buf); // Blocking call to get something into cache
	if (rc){return rc;}
	
	// Set bit to zero
	buf[charNumber] = buf[charNumber] & ~(0x1 << (7 - bitNumber));

	// Set dirty and release
	rc = rc | Set_Dirty(freeListBlockNo);
	rc = rc | Unfix_From_Cache(freeListBlockNo);
	rc = rc | Flush_Cache_Block(freeListBlockNo);
	return rc;
}
