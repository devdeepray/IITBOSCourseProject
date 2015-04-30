/*
 * Superblock initialization, formatting of disk, file system params.
 * Also acts as interface to copy form cache to workspace.
 */

#include <geekos/oscourse/blocks.h>
#include <geekos/string.h>
#include <geekos/synch.h>
#include <geekos/oscourse/oft.h>
#include <geekos/oscourse/dirmgmt.h>
#include <geekos/kassert.h>

#include <geekos/int.h>

struct SuperBlock disk_superblock;
struct Mutex free_list_lock;

// Writes block from working area to disk cache
int Write_Block(  int blockNo, char* data) {
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
int Read_Block(  int blockNo, char* dataDest) {
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
	//Enable_Interrupts();
	rc = Format_Super_Block();
	//Disable_Interrupts();
	if (rc){
		Print("blocks.c/Format_Disk: Could not format superblock\n");
		return rc;
	}
	// Format the free list bitmap
	//Enable_Interrupts();
	rc = Init_Free_List();
	//Disable_Interrupts();
	if (rc)
	{
		Print("blocks.c/Format_Disk: Free list initialization failed\n");
		return rc;
	}
	// Format the inode bitmap
//Enable_Interrupts();
	rc = Format_Inode_Blocks();
//Disable_Interrupts();
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
	rc = Init_Oft();
	if(rc)
	{
		Print("blocks.c/Init_File_System: Could not initialize the oft\n");
		return rc;
	}
	Print("File system initialized.\n");
	//Mutex_Init(&free_list_lock);
	return 0;
}

int Shut_Down_File_System()
{
	//~ Print("Deleting things in OFT\n");
	//~ int rc = Clean_Oft();
	//~ if(rc)
	//~ {
		//~ Print("blocks.c/Shut_Down_File_System: Could not shut down the OFT\n");
	//~ }
	
	Print("Shutting down file system...\n");
	int rc = Shut_Down_Inode_Manager() ;
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
		/ sizeof(Inode))) / (8 * disk_superblock.blockSizeInBytes) + 1;
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
	  int i, j;
	int rc = 0;
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
	 int i;
	int rc = 0;
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
	inode.meta_data.file_size = sizeof(DirHeader);
	inode.meta_data.group_id = 1; // all
	inode.meta_data.owner_id = 1; // root
	inode.meta_data.permissions = 0177; // All can read, group and owner can write/exec
	inode.meta_data.is_directory = 1;
	inode.entries[0] = disk_superblock.rootDirectoryBlock;
	//buf[0] = 0x00;   // Pratik: Is this needed?
	memcpy(buf, &inode, sizeof(Inode));
	
	rc = Write_To_Disk(buf, disk_superblock.firstInodeBlock, 1);
	if(rc)
	{
		Print("blocks.c/Format_Inode_Blocks: Could not write root inode to disk\n");
		return rc;
	}
	Init_Root_Dir();
	
	Print("Finished formatting Inode blocks.\n");
	return 0;
}

int Init_Root_Dir() {
	DirHeader roothead;
	roothead.numEntries = 0;
	roothead.parentInode = -1;
	
	char buf[BLOCK_SIZE];
	
	memcpy(buf, (void*)&roothead, sizeof(DirHeader));
	
	Write_To_Disk(buf,  disk_superblock.rootDirectoryBlock, 1);
	return 0;
	
}

// Traverses the free blocks bitmap and finds a free block 
int Allocate_Block(  int* freeBlock) {
	//Mutex_Lock(&free_list_lock);
	char* buf;
	  int i, j;
	int rc = 0;
	int flag = 0;
	
	// Loop to go over all bitmap blocks
	for(i = 0; i < disk_superblock.numFreeListBitmapBlocks && flag == 0; ++i)
	{
		rc = Get_Into_Cache(i + disk_superblock.firstFreeListBitmapBlock, &buf);
		if(rc) 
		{	
			//Mutex_Unlock(&free_list_lock);
			return rc; // Get into cache error. Fatal
			
		}
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
	//Mutex_Unlock(&free_list_lock);
	return rc;
}

/* Frees the given block number by setting the corresponding bit in the bitmap to 0 */
int Free_Block(  int blockNo) {
	//Mutex_Lock(&free_list_lock);
	if(blockNo > disk_superblock.totBlocks){
		//Mutex_Unlock(&free_list_lock); 
		return -1;
	}
	// Calculate the block number for the corresponding bit
	int freeListBlockNo = disk_superblock.firstFreeListBitmapBlock + (blockNo / (disk_superblock.blockSizeInBytes * 8));
	// Calculate char number for corresponding bit
	int charNumber = (blockNo % disk_superblock.blockSizeInBytes) / 8;
	// Calculate bit number for corresponding bit
	int bitNumber = blockNo % 8;
	
	char *buf;
	int rc = Get_Into_Cache(freeListBlockNo, &buf); // Blocking call to get something into cache
	if (rc){
		//Mutex_Unlock(&free_list_lock);
		return rc;
	}
	
	// Set bit to zero
	buf[charNumber] = buf[charNumber] & ~(0x1 << (7 - bitNumber));

	// Set dirty and release
	rc = rc | Set_Dirty(freeListBlockNo);
	rc = rc | Unfix_From_Cache(freeListBlockNo);
	rc = rc | Flush_Cache_Block(freeListBlockNo);
	//Mutex_Unlock(&free_list_lock);
	return rc;
}
