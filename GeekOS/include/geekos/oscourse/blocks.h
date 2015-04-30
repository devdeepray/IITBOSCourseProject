/*
 * Superblock initialization, formatting of disk, file system params.
 * Also acts as interface to copy form cache to workspace.
 */
#ifndef _BLOCKS_H_
#define _BLOCKS_H_
 
#include <geekos/oscourse/fsysdef.h>
#include <geekos/oscourse/virtualdisk.h>
#include <geekos/oscourse/diskcache.h>
#include <geekos/oscourse/inode.h>

typedef struct SuperBlock {
	/* Disk parameters */
	   int blockSizeInBytes;
	   int totBlocks;

	/* FS parameters */
	   int diskCacheSize;
	   int numAllocatedBlocks;
	   int firstFreeListBitmapBlock;
	   int numFreeListBitmapBlocks;
	   int firstInodeBitmapBlock;
	   int numInodeBitmapBlocks;
	   int firstInodeBlock;
	   int numInodeBlocks;
	   int rootDirectoryBlock;
} SuperBlock;

extern SuperBlock disk_superblock;

int Write_Block(   int, char*);

int Read_Block(   int, char*);

int Format_Disk();

int Init_Root_Dir();

int Init_File_System();

int Shut_Down_File_System();

int Allocate_Block(   int*);

int Free_Block(   int);

int Format_Super_Block();

int Init_Free_List();

int Format_Inode_Blocks();

int Read_Super_Block();

int Write_Super_Block();

#endif
