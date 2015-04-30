#include <geekos/oscourse/diskcache.h>
#include <geekos/oscourse/fsysdef.h>
#include <geekos/screen.h>
#include <geekos/synch.h>
#include <geekos/malloc.h>
CachePage *first_page;
CachePage *last_page;
CachePage *first_free_page;
CachePage *last_free_page;

struct Mutex cache_lock;

int current_cache_size;
Hashtable cache_hash;

// Initializes disk cache
int Init_Disk_Cache()
{
	Print("Initializing disk cache...\n");
	current_cache_size = 0;
	first_page = NULL;
	last_page = NULL;
	first_free_page = NULL;
	last_free_page = NULL;
	int rc = Init_Hash_Table(&cache_hash, DISK_CACHE_SIZE, DISK_CACHE_HASH_MULT);
	if(rc)
	{
		Print("diskcache.c/Init_Disk_Cache: Hash table initialization failed");
		return rc;
	}
	Print("Disk cache initialized.\n");
	//Mutex_Init(&cache_lock);
	return 0;
}


int Shut_Down_Disk_Cache()
{
	Print("Shutting down disk cache...\n");
	int grc = 0;
	int rc = Flush_Cache();
	grc = rc | grc;
	if(rc)
	{
		Print("diskcache.c/Shut_Down_Disk_Cache: Could not flush the cache\n");
	}
	rc = Free_Cache();
	grc = rc | grc;
	if(rc)
	{
		Print("diskcache.c/Shut_Down_Disk_Cache: Could not free the cache memory\n");
	}
	rc = Clear_Hash_Table(&(cache_hash));
	grc = rc | grc;
	if(rc)
	{
		Print("diskcache.c/Shut_Down_Disk_Cache: Could not free the cache hash\n");
	}
	Print("Shut down disk cache. \n");
	return grc;
}

int Free_Cache()
{
//Mutex_Lock(&cache_lock);
	CachePage* cur = first_free_page;
	while(cur != NULL)
	{
		CachePage* next = cur->next;
		Free(cur);
		cur = next;
	}
	cur = first_page;
	while(cur != NULL)
	{
		CachePage* next = cur->next;
		Free(cur);
		cur = next;
	}
//Mutex_Unlock(&cache_lock);
	return 0;
}


// Gets a page into cache, or if it is there, icreases refcount
int Get_Into_Cache(int blockNo, char** buf)
{
	//Mutex_Lock(&cache_lock);
	CachePage *page;
	int rc = Get_From_Hash_Table(&cache_hash, blockNo, (void**) &page);
	if(rc == 0)
	{
		if(page->ref_count == 0) // In the free list
		{
			Unlink_From_Free(page);
			Link_To_Cache(page);
		}
		else // Else bring it to head
		{
			Unlink_From_Cache(page);
			Link_To_Cache(page);
		}
			
		// Already exists
		page->ref_count++;
		(*buf) = page->buf;
	}
	else
	{
		// Need to fetch from disk
		CachePage *rep_page;
		if(current_cache_size < DISK_CACHE_SIZE) // Cache is still small
		{
			// Allocate new cache page in the free list
			rep_page = (CachePage*) Malloc(sizeof(CachePage));
			rep_page->ref_count = 0;
			rep_page->dirty = 0;
			rep_page->block_no = -1; // Not a valid block
			rep_page->prev = NULL;
			rep_page->next = NULL;
			Link_To_Free(rep_page);
			current_cache_size++;
			Print("Cache Grown To %d\n",current_cache_size);
		}
		else // Cache size is max
		{
			// Find replacement, write back if dirty, and load new page
		
			rc = Find_Replacement(&rep_page); // Gets replacement
			if(rc) goto cleanAndReturn; // Cache is full
			rc = Write_If_Dirty(rep_page); // Cleans page
			if(rc) goto cleanAndReturn; // Write fail
			int rc = Remove_From_Hash_Table(&cache_hash, rep_page->block_no);
			if(rc) goto cleanAndReturn; // Not in hash. should not occur
		}
		// Read actual contents
		rc = Read_From_Disk(rep_page->buf, blockNo, 1);
		if(rc) goto cleanAndReturn; // Read failed
		// Add new entry to hash
		rc = Add_To_Hash_Table(&cache_hash, blockNo, (void**) rep_page);
		if(rc) goto cleanAndReturn;
		rep_page->block_no = blockNo; // Set block number
		Unlink_From_Free(rep_page); // Remove from free
		Link_To_Cache(rep_page); // Add to main cache
		rep_page->ref_count = 1; // make refcount 1
		(*buf) = rep_page->buf;
	}
	cleanAndReturn:
	//Mutex_Unlock(&cache_lock);
	return 0;
}

int Unfix_From_Cache(int blockNo)
{
//Mutex_Lock(&cache_lock);
	CachePage* page;
	int rc = Get_From_Hash_Table((&cache_hash), blockNo, (void**)(&page));
	if(rc){
		//Mutex_Unlock(&cache_lock); return rc;
	}
	if(page->ref_count == 0) {
		//Mutex_Unlock(&cache_lock);
		return -1; // Cannot unfix
	}
	page->ref_count--;
	if(page->ref_count == 0)
	{
		Unlink_From_Cache(page);
		Link_To_Free(page);
	}
	//Mutex_Unlock(&cache_lock);
	return 0;
}


// Set page corresponding to blockNo to dirty
int Set_Dirty(int blockNo)
{
	//Mutex_Lock(&cache_lock);
	CachePage* page;
	int rc = Get_From_Hash_Table(&cache_hash, blockNo, (void**)(&page));
	if(rc) return rc;
	page->dirty = 1;
	//Mutex_Unlock(&cache_lock);
	return 0;
	
}

// Flush cache and make all pages clean
int Flush_Cache()
{	
	//Mutex_Lock(&cache_lock);
	int i;
	CachePage* cur = first_page;
	int rc = 0;
	while(cur != NULL)
	{
		rc = rc | Write_If_Dirty(cur);
		cur = cur->next;
	}
	cur = first_free_page;
	int count = 0;
	while(cur != NULL && ++count < 5)
	{
		rc = rc | Write_If_Dirty(cur);
		cur = cur->next;
	}
	//Mutex_Unlock(&cache_lock);
	return rc;
}

// Flush single page
int Flush_Cache_Block(int blockNo)
{
	//Mutex_Lock(&cache_lock);
	CachePage* page;
	int rc = Get_From_Hash_Table(&cache_hash, blockNo, (void**)&page);
	if(rc)
	{
		//Mutex_Unlock(&cache_lock);
		return 0;
	}
	//Mutex_Unlock(&cache_lock);
	return Write_If_Dirty(page);
}
	
	
	
// Finds a replacement from the free list
int Find_Replacement(CachePage **rep_page)
{	
	// No free page available
	if(last_free_page == NULL) return -1;
	
	// Set cur 
	*rep_page = last_free_page;
	Print("Replaced Page is %d\n", (*rep_page)->block_no);
	return 0;
}

// Writes a page if it is dirty
int Write_If_Dirty(CachePage *rep_page)
{
	int rc;
	if(rep_page->dirty)
	{
		rc = Write_To_Disk(rep_page->buf, rep_page->block_no, 1);
		if(rc) return rc; // FATAL
		rep_page->dirty = 0; // Set to clean
	}
	return 0;
}

// Adds page to the front of free list
void Link_To_Free(CachePage* page)
{
	page->next = first_free_page;
	if(first_free_page != NULL)
	{
		first_free_page->prev = page;
	}else{
		last_free_page = page;
	}
	first_free_page = page;
	page->prev = NULL;
}

// Adds page to the front of cache list
void Link_To_Cache(CachePage* page)
{
	page->next = first_page;
	if(first_page != NULL)
	{
		first_page->prev = page;
	}else{
		last_page = page;
	}
	
	first_page = page;
	page->prev = NULL;
}

// Removes page from arbit position in the free list
void Unlink_From_Free(CachePage* page)
{
	// Three cases, at the head, at the tail and in between
	if(page->prev == NULL) // at head
	{
		first_free_page = page->next;
		if(page->next != NULL)
		{
			page->next->prev = NULL;
		}
		page->next = NULL;
	}
	else if(page->next == NULL) // at end
	{
		last_free_page = page->prev;
		page->prev->next = NULL;
		page->prev = NULL;
	}
	else
	{
		page->next->prev = page->prev;
		page->prev->next = page->next;
		page->next = NULL;
		page->prev = NULL;
	}
}

// Removes page from arbit position in the cache list
void Unlink_From_Cache(CachePage* page)
{
	// Three cases, at the head, at the tail and in between
	if(page->prev == NULL) // at head
	{
		first_page = page->next;
		if(page->next != NULL)
		{
			page->next->prev = NULL;
		}
		page->next = NULL;
	}
	else if(page->next == NULL) // at end
	{
		last_page = page->prev;
		page->prev->next = NULL;
		page->prev = NULL;
	}
	else
	{
		page->next->prev = page->prev;
		page->prev->next = page->next;
		page->next = NULL;
		page->prev = NULL;
	}
}

	



