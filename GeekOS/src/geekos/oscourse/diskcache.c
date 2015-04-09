#include <diskcache.h>
#include <fsysdef.h>

CachePage *first_page;
CachePage *last_page;
CachePage *first_free_page;
CachePage *last_free_page;

int current_cache_size;
Hashtable cache_hash;

// Initializes disk cache
int Init_Disk_Cache()
{
	current_cache_size = 0;
	first_page = NULL;
	last_page = NULL;
	first_free_page = NULL;
	last_free_page = NULL;
	int rc = Init_Hash_Table(cache_hash, DISK_CACHE_SIZE, DISK_CACHE_HASH_MULT);
	return rc;
}

// Gets a page into cache, or if it is there, icreases refcount
int Get_Into_Cache(int blockNo, char** buf)
{
	CachePage *page;
	int rc = Get_From_Hash_Table(&cache_hash, blockNo, (void**) &page);
	if(rc == 0)
	{
		if(ref_count == 0) // In the free list
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
			rep_page->refcount = 0;
			rep_page->dirty = 0;
			rep_page->block_no = -1; // Not a valid block
			Link_To_Free(rep_page);
			current_cache_size++;
		}
		else // Cache size is max
		{
			// Find replacement, write back if dirty, and load new page
		
			rc = Find_Replacement(&rep_page); // Gets replacement
			if(rc) return rc; // Cache is full
			rc = Write_If_Dirty(rep_page); // Cleans page
			if(rc) return rc; // Write fail
			int rc = Remove_From_Hash_Table(&cache_hash, rep_page->block_no);
			if(rc) return rc; // Not in hash. should not occur
		}
		
		// Read actual contents
		rc = Read_From_Disk(rep_page->buf, blockNo, 1);
		if(rc) return rc; // Read failed
		
		// Add new entry to hash
		rc = Add_To_Hash_Table(&cache_hash, blockNo, (void**) rep_page);
		if(rc) return rc;
		
		rep_page->block_no = blockNo; // Set block number
		Unlink_From_Free(rep_page); // Remove from free
		Link_To_Cache(rep_page); // Add to main cache
		rep_page->ref_count = 1; // make refcount 1
		(*buf) = rep_page->buf;
	}
	return 0;
}

int Unfix_From_Cache(int blockNo)
{
	CachePage* page;
	int rc = Get_From_Hash_Table(&cache_hash, blockNo, &page);
	if(rc) return rc;
	if(page->ref_count == 0) return -1; // Cannot unfix
	page->ref_count--;
	if(ref_count == 0)
	{
		Unlink_From_Cache(page);
		Link_To_Free(page);
	}
	return 0;
}


// Set page corresponding to blockNo to dirty
int Set_Dirty(int blockNo)
{
	CachePage* page;
	int rc = Get_From_Hash_Table(&cache_hash, blockNo, &page);
	if(rc) return rc;
	page->dirty = 1;
	return 0;
}

// Flush cache and make all pages clean
int Flush_Cache()
{	
	int i;
	CachePage* cur = first_page;
	int rc = 0;
	while(cur != NULL)
	{
		rc = rc | Write_If_Dirty(cur);
		cur = cur->next;
	}
	cur = first_free_page;
	while(cur != NULL)
	{
		rc = rc | Write_If_Dirty(cur);
		cur = cur->next;
	}
	return rc;
}

// Flush single page
int Flush_Cache_Block(int blockNo)
{
	CachePage* page;
	int rc = Get_From_Hash_Table(&cache_hash, blockNo, &page);
	if(rc) return 0;
	return Write_If_Dirty(pageq);
}
	
	
	
// Finds a replacement from the free list
int Find_Replacement(CachePage **rep_page)
{
	// No free page available
	if(last_free_page == NULL) return -1;
	
	// Set cur 
	*rep_page = last_free_page;
	return 0;
}

// Writes a page if it is dirty
int Write_If_Dirty(CachePage *page)
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
Link_To_Free(CachePage* page)
{
	page->next = first_free_page;
	first_free_page->prev = page;
	first_free_page = page;
	page->prev = NULL;
}

// Adds page to the front of cache list
Link_To_Cache(CachePage* page)
{
	page->next = first_page;
	first_page->prev = page;
	first_page = page;
	page->prev = NULL;
}

// Removes page from arbit position in the free list
Unlink_From_Free(CachePage* page)
{
	// Three cases, at the head, at the tail and in between
	if(page->prev == NULL) // at head
	{
		first_free_page = page->next;
		page->next = NULL;
	}
	else if(page->next == NULL) // at end
	{
		last_free_page = page->prev;
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
Unlink_From_Cache(CachePage* page)
{
	// Three cases, at the head, at the tail and in between
	if(page->prev == NULL) // at head
	{
		first_page = page->next;
		page->next = NULL;
	}
	else if(page->next == NULL) // at end
	{
		last_page = page->prev;
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

	



