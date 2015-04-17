/*
 * Superblock initialization, formatting of disk, file system params.
 * Also acts as interface to copy form cache to workspace.
 */
#ifndef _BLOCKS_H_
#define _BLOCKS_H_
 
#include "fsysdef.h"
#include "virtualdisk.h"
#include "diskcache.h"
#include "inode.h"

typedef struct SuperBlock {
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
} SuperBlock;

extern SuperBlock disk_superblock;

int Write_Block(int, char*);

int Read_Block(int, char*);

int Format_Disk();

int Init_File_System();

int Shut_Down_File_System();

int Allocate_Block(int*);

int Free_Block(int);

int Format_Super_Block();

int Init_Free_List();

int Format_Inode_Blocks();

int Read_Super_Block();

int Write_Super_Block();

#endif
