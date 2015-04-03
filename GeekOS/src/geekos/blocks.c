#include <geekos/blocks.h>

superblock disk_superblock;

/*
	TODO this
		1) Handle all error codes
		2) Decide disk and file cache sizes
	TODO IOCS
		1) waitGetBlock - will definitely get a disk block (will block if needed) and fix it in the disk cache
		2) setDirtyBit - will set the dirty bit of the given blockNo in the disk cache (This has to be in the cache as this has not been yet unfixed)
		3) unFix - unfixes a block so that it can now be considered for eviction if needed.
		4) getBlockSizeInBytes()
		5) getTotBlocks()
	TODO inode
*/


// Writes block from working area to disk cache
int Write_Block(int blockNo, char* data) {
	char* block;
	int rc = 0;
	rc = Get_Into_Cache(int blockNo, &block);
	if (rc){return rc;}
	memcpy((void*)block, data, disk_superblock.blockSizeInBytes);
	rc = Set_Dirty(blockNo);
	if(rc) { return rc;}
	rc = Unfix_From_Cache(blockNo);
	return rc;
}

// Reads block from disk cache to working area
int Read_Block(int blockNo, char* dataDest) {
	char* block;
	int rc = Get_Into_Cache(int blockNo, &block);
	if (rc){return rc;}
	memcpy((void*)dataDest, (void*)block, disk_superblock.blockSizeInBytes);
	Unfix_From_Cache(blockNo);
	return 0;
}

// Added a syscall for this
// Formats a disk and then initializes sim disk
int Format_Disk() {
	int rc;
	rc = Format_Super_Block();
	if (rc) {return rc;}
	rc = Format_Free_List();
	if (rc) {return rc;}
	rc = Format_Inode_Blocks();
	if(rc) return rc;
	rc = Init_File_System();
	return rc;
}

// Initializes the file system
int Init_File_System() {
	int rc = 0;
	rc = Read_Super_Block();
	if (rc) {return rc;}
	rc = Init_Disk_Cache();
	if (rc) {return rc;}
	rc = Init_OFT();
	if(rc) return rc;
	rc = Init_Inode_Manager();
	if (rc) {return rc;}
	return 0;
}

int Format_Super_Block() {
	disk_superblock.blockSizeInBytes = disk_hw_data.bytes_per_block;
	disk_superblock.totBlocks = disk_hw_data.blocks_per_track 
							* disk_hw_data.tracks_per_cylinder 
							* disk_hw_data.tot_cylinders;
	disk_superblock.diskCacheSize = 20;
	disk_superblock.fileCacheSize = 10;


	disk_superblock.firstFreeListBlock = 1;
	disk_superblock.numFreeListBlocks = ((disk_superblock.totBlocks) / (disk_superblock.blockSizeInBytes * 8)) + 1;
	disk_superblock.firstInodeBitmapBlock = disk_superblock.firstFreeListBlock + disk_superblock.numFreeListBlocks;
	disk_superblock.numInodeBlocks = 20;
	disk_superblock.numInodeBitmapBlocks = (disk_superblock.numInodeBlocks * disk_superblock.blockSizeInBytes / sizeof(Inode)) / (8 * disk_superblock.blockSizeInBytes) + 1;
	disk_superblock.firstInodeBlock = disk_superblock.firstInodeBitmapBlock + disk_superblock.numInodeBitmapBlocks;
	disk_superblock.numAllocatedBlocks = disk_superblock.firstInodeBlock + disk_superblock.numInodeBlocks;

	// Write the superblock to disk after formating (?)
	Write_Super_Block();
}

int Read_Super_Block() {
	char block[BLOCK_SIZE];
	int rc = Read_From_Disk(block, SUPERBLOCKNO, 1);
	if (rc){return rc;}
	memcpy((void*) &disk_superblock, (void*) block, sizeof(superblock));
	return 0; 
}

int Write_Super_Block() {
	char block[BLOCK_SIZE];
	if (rc){return rc;}
	memcpy((void*) block, (void*) &disk_superblock, sizeof(superblock));
	Write_To_Disk(block, SUPERBLOCKNO, 1);
	return 0;
}

/*
	bitwise data
	assumes that no data exists when this is called 
*/
int Init_Free_List() {
	
	char buf[BLOCK_SIZE];
	char bitmapArr[] = {0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF}; 
	int i, j;
	for(i = 0; i < BLOCK_SIZE; ++i)
	{
		buf[i] = 0;
	}
	
	for(i = 0; i < disk_superblock.numFreeListBitmapBlocks; ++i)
	{
		Write_To_Disk(buf, i + disk_superblock.firstFreeListBitmapBlock, 1);
	}
	
	for(i = 8, j = 0; i < disk_superblock.numAllocatedBlocks; i += 8, ++j)
	{
		buf[j] = 0xFF;
	}
	
	int numOnes = 8 - (i - disk_superblock.numAllocatedBlocks);
	buf[j] = bitmapArr[numOnes];
	Write_To_Disk(buf, disk_superblock.firstFreeListBitmapBlock, 1);
	
}

/* Traverses the free blocks bitmap and finds a free block */
int Allocate_Block(int* freeBlock) {
	
	char* buf;
	int i, j;
	int flag = 0;
	for(i = 0; i < disk_superblock.numFreeListBitmapBlocks && flag == 0; ++i)
	{
		Get_Into_Cache(i + disk_superblock.firstFreeListBitmapBlock, &buf);
		for(j = 0; j < disk_superblock.blockSizeInBytes && flag == 0; ++j)
		{
			if(buf[j] != 0xFF)
			{
				flag = 1;
				int k;
				for(k = 0; k < 8; ++k)
				{
					if((buf[j] | 0x1 << (7 - k))  != buf[j])
					{
						buf[j] = buf[j] | 0x1 << (7 - k);
						*freeBlock = (disk_superblock.blockSizeInBytes * i + j) * 8 + k;
						Set_Dirty_Cache(i + disk_superblock.firstFreeListBitmapBlock);
					}
				}
			}
		}
		Unfix_From_Cache(i + disk_superblock.firstFreeListBitmapBlock);	
	}
}

/* Frees the given block number by setting the corresponding bit in the bitmap to 0 */
int Free_Block(int blockNo) {
	int freeListBlockNo = disk_superblock.firstFreeListBlock + (blockNo / (disk_superblock.blockSizeInBytes * 8));
	int charNumber = (blockNo % disk_superblock.blockSizeInBytes) / 8;
	int bitNumber = blockNo % 8;
	
	char *buf;
	int rc = Get_Into_Cache(freeListBlockNo, buf);
	if (rc){return rc;}
	buf[charNumber] = buf[charNumber] & ~(0x1 << (7 - bitNumber));
	setDirtyBit(freeListBlockNo);
	Unfix_From_Cache(freeListBlockNo);
}
