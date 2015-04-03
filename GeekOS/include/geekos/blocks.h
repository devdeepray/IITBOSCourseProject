/* Superblock struct */
/* This will be located at the first block of the disk */
struct superblock {
	/* Disk parameters */
	unsigned int blockSizeInBytes;
	unsigned int totBlocks;

	/* FS parameters */
	unsigned int diskCacheSize;
	unsigned int fileCacheSize;
	unsigned int numAllocatedBlocks;
	unsigned int firstFreeListBitmapBlock;
	unsigned int numFreeListBitmapBlocks;
	unsigned int firstInodeBitmapBlock;
	unsigned int numInodeBitmapBlocks;
	unsigned int firstInodeBlock;
	unsigned int numInodeBlocks;
};

#define SUPERBLOCKNO 0

extern superblock disk_superblock;

int writeIntoBlock(int blockNo, char* data);
int readIntoMem(int blockNo, char* dataDest, int *blockSize);
int FormatSimDisk(int noBlocks, int blockSize, int fileCacheSize, int diskCacheSize);
int initSimDisk();
int initSuperBlock(int noBlocks, int blockSize, int fileCacheSize, int diskCacheSize);
int readSuperBlock();
int writeSuperBlock();
int initFreeList();
int allocateBlock(int* freeBlock);
int freeBlock(int blockNo);
