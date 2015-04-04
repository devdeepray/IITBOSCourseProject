/*
 * Superblock initialization, formatting of disk, file system params.
 * Also acts as interface to copy form cache to workspace.
 */
#include <geekos/oscourse/fsysdef.h>

struct SuperBlock {
	/* Disk parameters */
	unsigned int blockSizeInBytes;
	unsigned int totBlocks;

	/* FS parameters */
	unsigned int diskCacheSize;
	unsigned int numAllocatedBlocks;
	unsigned int firstFreeListBitmapBlock;
	unsigned int numFreeListBitmapBlocks;
	unsigned int firstInodeBitmapBlock;
	unsigned int numInodeBitmapBlocks;
	unsigned int firstInodeBlock;
	unsigned int numInodeBlocks;
	unsigned int rootDirectoryBlock;
};

extern SuperBlock disk_superblock;

int Write_Block(int, char*);

int Read_Block(int, char*);

int Format_Disk();

int Init_File_System();

int Allocate_Block(int*);

int Free_Block(int);
