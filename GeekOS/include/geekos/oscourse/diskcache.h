#ifndef _DISKCACHE_H_
#define _DISKCACHE_H_

#include <geekos/oscourse/hash.h>
#include <geekos/oscourse/fsysdef.h>
#include <geekos/oscourse/blocks.h>

typedef struct CachePage {
	char buf[BLOCK_SIZE];
	int block_no;
	int ref_count;
	int dirty;
	struct CachePage *next;
	struct CachePage *prev;
} CachePage;



int Init_Disk_Cache();

int Shut_Down_Disk_Cache();

int Get_Into_Cache(int blockNo, char** buf);

int Set_Dirty(int blockNo);

int Unfix_From_Cache(int blockNo);

int Flush_Cache();

int Free_Cache();

int Flush_Cache_Block(int blockNo);

void Unlink_From_Cache(CachePage*);

void Unlink_From_Free(CachePage*);

void Link_To_Cache(CachePage*);

void Link_To_Free(CachePage*);

int Find_Replacement(CachePage**);

int Write_If_Dirty(CachePage*);

#endif
