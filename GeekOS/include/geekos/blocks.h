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
	unsigned int firstFreeListBlock;
	unsigned int numFreeListBlocks;
	unsigned int firstInodeBlock;
	unsigned int numInodeBlocks;
};

#define SUPERBLOCKNO 0

extern superblock disk_superblock;

int writeIntoBlock(int, char*);
int readIntoMem(int, char*, int*);
int initSuperBlock();
int readSuperBlock();
int writeSuperBlock();
int initFreeList();
int allocateBlock(int*);
int freeBlock(int);