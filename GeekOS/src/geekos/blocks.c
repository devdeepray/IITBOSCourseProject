#include <geekos/blocks.h>


superblock disk_superblock;

int writeIntoBlock(int blockNo, char* data) {
	// Release buffer, handle return codes, 	
	char* block;
	int rc = waitGetBlock(int blockNo, &block); // Definitely get a block
	memcpy((void*)block, data, disk_superblock.blockSizeInBytes));
	
	return 0;
}

int readIntoMem(int blockNo, char** data, int *blockSize) {
	// Change it so that data is copied from cache to workspace
	// Handle cache locks and return code
	char* block = getBlock(int blockNo); // TODO IOCS

	struct blockMetaData *metadata;
	metadata = (blockMetaData*) block;
	
	*data =  (char*)(block + metadata->startByte);
	*dataRem = superblock.blockSizeInBytes - metadata->startByte
	return 0;
}



/*
file structure on different lines
	int block_size,
	int tot_blocks;
	int tot_tracks;
	int track_cap;
	int time_to_shift_cyl;
	int rot_time;
	int block_read_time;
	int diskcachesize
	int filecachesize
*/


int initSuperBlock(char* initFilePath, *superblock) {
	struct File *file_struct;
 	int rc = Open(DISK_CONFIG_FILE, 0, &file_struct);
	Print("%d", rc);
	fileSize = Read(file_struct, file, MAXFILESIZE);
	Close(file_struct);
	Print("In Init sim disk");

	char buf[20];
	readLineFileArray(buf);
	superblock->blockSizeInBytes = atoi(buf);

	readLineFileArray(buf);
	superblock->totBlocks = atoi(buf);

	readLineFileArray(buf);
	superblock->totTracksPerCyl = atoi(buf);

	readLineFileArray(buf);
	superblock->trackCap = atoi(buf);

	readLineFileArray(buf);
	superblock->timeToShiftCyl = atoi(buf);

	readLineFileArray(buf);
	superblock->rotTime = atoi(buf);

	readLineFileArray(buf);
	superblock->blockReadTime = atoi(buf);

	readLineFileArray(buf);
	superblock->diskCacheSize = atoi(buf);

	readLineFileArray(buf);
	superblock->fieCacheSize = atoi(buf);

	superblock->rootDirBlock = 2;

	superblock->currentDiskSize = ;  //
	superblock->freeListBlock = 1;

	initFreeList(2); // Error handling

	/* Data about metadata in a normal data block */
	superblock->startMetaDataSize = sizeof(blockMetaData);
	superblock->endMetaDataSize = 0;
}



int readSuperBlock(*superblock) {
	memcpy((void*) superblock, getBlock(0), sizeof(superblock)); // TODO getBlock()
	return 0; 
}



/*
	bitwise data
	
*/

int initFreeList(int block) {
	freeList = malloc(superblock.totBlocks * sizeof(unsigned char));

	char* blockPtr = getBlock(block);
	int i = 0;
	for (; i < superblock.totBlocks; ++i) {
		blockPtr[i] = '\0';
	}
}

int allocateBlock(int* freeBlock) {

	int j = 1;
	j = j << sizeof(unsigned char) - 1;



	char* freeList = getBlock(superblock.freeListBlock);

	int i = 0;

	while (freeList[i] == j)  {
		++i;
	}

	*freeBlock = sizeof(unsigned char) * i;

	char bitmap = freeList[i];
	while (bitmap && 1) {
		bitmap = bitmap >> 1;
		*freeBlock++;
	}

	return 0;
}



int freeBlock(int block) {
	int loc = (block / sizeof(unsigned char));

	char* freeList = getBlock(superblock.freeListBlock);

	freelist[loc] = freelist[loc] & (~(1 << (block % sizeof(unsigned char))));

}
