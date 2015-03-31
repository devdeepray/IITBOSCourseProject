/* Superblock struct */


/* This will be located at the first block of the disk */
struct superblock {

	/* Disk parameters */
	int blockSizeInBytes;
	int totBlocks;

	/* FS parameters */
	int diskCacheSize;
	int fileCacheSize;
	int numAllocatedBlocks;
	int firstFreeListBlock;
	int numFreeListBlocks;
	int firstInodeBlock;
	int numInodeBlocks;

};

extern superblock disk_superblock;

int initSuperBlock(char* initFilePath, *superblock);
int readSuperBlock(*superblock);
