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

int Write_Block(int, char*);

int Read_Block(int, char*);

int Format_Disk();

int Init_File_System();

int Format_Super_Block();

int Read_Super_Block();

int Write_Super_Block();

int Init_Free_List();

int Allocate_Block(int*);


int Free_Block(int);
