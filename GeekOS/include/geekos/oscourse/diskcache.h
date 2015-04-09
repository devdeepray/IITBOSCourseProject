#include <geekos/oscourse/hash.h>
#include <geekos/oscourse/fsysdef.h>

typedef struct CachePage_s {
	char buf[BLOCK_SIZE];
	int block_no;
	int ref_count;
	int dirty;
	CachePage *next;
	CachePage *prev;
} CachePage;



int Init_Disk_Cache();

int Get_Into_Cache(int blockNo, char** buf);

int Set_Dirty(int blockNo);

int Unfix_From_Cache(int blockNo);

int Flush_Cache();

int Flush_Cache_Block(int blockNo);
