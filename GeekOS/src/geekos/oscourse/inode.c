#include <geekos/oscourse/inode.h>
#include <geekos/malloc.h>
#include <geekos/synch.h>

InodeManager inode_manager;
struct Mutex inode_lock;
struct Mutex inode_cache_lock;

int Init_Inode_Manager() {
		
	inode_manager.bitmap_start_block = disk_superblock.firstInodeBitmapBlock;
	inode_manager.inode_start_block = disk_superblock.firstInodeBlock;
	inode_manager.bitmap_number_of_blocks = disk_superblock.numInodeBitmapBlocks;
	inode_manager.inode_number_of_blocks = disk_superblock.numInodeBlocks;
	
	
	inode_manager.current_cache_size = 0;
	inode_manager.first_free_inode = NULL;
	inode_manager.last_free_inode = NULL;	
	inode_manager.first_inode = NULL;
	inode_manager.last_inode = NULL;

	int rc = Init_Inode_Cache();

	Mutex_Init(&inode_lock);
	Mutex_Init(&inode_cache_lock);

	return rc;
}

int Shut_Down_Inode_Manager()
{
	Print("Shutting down inode manager...\n");
	int grc = 0;
	int rc = Flush_Inode_Cache();
	grc = rc | grc;
	if(rc)
	{
		Print("inode.c/Shut_Down_Inode_Manager: Could not flush the inode cache\n");
	}
	rc = Free_Inode_Cache();
	grc = rc | grc;
	if(rc)
	{
		Print("inode.c/Shut_Down_Inode_Manager: Could not free the inode cache memory\n");
	}
	rc = Clear_Hash_Table(&(inode_manager.cache_hash));
	grc = rc | grc;
	if(rc)
	{
		Print("inode.c/Shut_Down_Inode_Manager: Could not free the inode cache hash\n");
	}
	Print("Shut down inode manager. \n");
	return grc;
}
	
int Free_Inode_Cache()
{
	Mutex_Lock(&inode_cache_lock);
	CacheInode* cur = inode_manager.first_free_inode;
	while(cur != NULL)
	{
		CacheInode* next = cur->next;
		Free(cur);
		cur = next;
	}
	cur = inode_manager.first_inode;
	while(cur != NULL)
	{
		CacheInode* next = cur->next;
		Free(cur);
		cur = next;
	}
	Mutex_Unlock(&inode_cache_lock);
	return 0;
}

	
// Initializes disk cache
int Init_Inode_Cache()
{
	inode_manager.current_cache_size = 0;
	inode_manager.first_inode = NULL;
	inode_manager.last_inode = NULL;
	inode_manager.first_free_inode = NULL;
	inode_manager.last_free_inode = NULL;
	int rc = Init_Hash_Table(&(inode_manager.cache_hash), INODE_CACHE_SIZE, INODE_CACHE_HASH_MULT);
	return rc;
}

// USER SPACE
// Traverses the free blocks bitmap and finds a free block 
int Allocate_Inode(int* freeInode) {
	char* buf;
	int i, j, rc = 0;
	int flag = 0;
	// Loop to go over all bitmap blocks
	for(i = 0; i < disk_superblock.numInodeBitmapBlocks && flag == 0; ++i)
	{
		   
		rc = Get_Into_Cache(i + disk_superblock.firstInodeBitmapBlock, &buf);
		   
		if(rc)
		{
			 return rc; // Get into cache error. Fatal
		}
		// Loop to go over all chars in block
		for(j = 0; j < disk_superblock.blockSizeInBytes && flag == 0; ++j)
		{
			if(buf[j] != (char)0xFF) // If found a 0 bit
			{
				
				flag = 1;
				int k;
				// Loop to go over bits in a char
				for(k = 0; k < 8; ++k)
				{
					   
					if((buf[j] | (char)0x1 << (7 - k))  != buf[j])
					{
						buf[j] = buf[j] | (char)0x1 << (7 - k);
						int freeInodeNum = (disk_superblock.blockSizeInBytes * i + j) * 8 + k;
						if(freeInodeNum > disk_superblock.numInodeBlocks * (BLOCK_SIZE /(int)sizeof(Inode)))
						{
							rc = -1;
						}
						*freeInode = freeInodeNum;
						if(!rc)
						{
							rc = Set_Dirty(i + disk_superblock.firstInodeBitmapBlock);
						}
						break;
					}
				}
			}
			if(flag) break;
		}
		
		rc = Unfix_From_Cache(i + disk_superblock.firstInodeBitmapBlock) | rc;
		if(flag && !rc) rc = Flush_Cache_Block(i + disk_superblock.firstInodeBitmapBlock);	
	}
	return rc;
	
}

/* Frees the given inode number by setting the corresponding bit in the bitmap to 0 */
int Free_Inode(int inodeNo) {
	Mutex_Lock(&inode_lock);
	if(inodeNo > disk_superblock.numInodeBlocks * (BLOCK_SIZE /(int)sizeof(Inode)))
	{
		return -1;
	}
	// Calculate the block number for the corresponding bit
	int inodeBlockNo = disk_superblock.firstInodeBitmapBlock + (inodeNo / (disk_superblock.blockSizeInBytes *(int)sizeof(char)));
	// Calculate char number for corresponding bit
	int charNumber = (inodeNo % disk_superblock.blockSizeInBytes) /(int)sizeof(char);
	// Calculate bit number for corresponding bit
	int bitNumber = inodeNo % 8;
	
	char *buf;
	int rc = Get_Into_Cache(inodeBlockNo, &buf); // Blocking call to get something into cache
	if (rc){
		return rc;
	}
	
	// Set bit to zero
	buf[charNumber] = buf[charNumber] & ~(0x1 << (7 - bitNumber));

	// Set dirty and release
	rc = rc | Set_Dirty(inodeBlockNo);
	rc = rc | Unfix_From_Cache(inodeBlockNo);
	rc = rc | Flush_Cache_Block(inodeBlockNo);
	Mutex_Unlock(&inode_lock);
	return rc;
}


int Create_New_Inode(InodeMetaData meta_data, int* newInodeNo) {
	Inode newInode;
	newInode.meta_data = meta_data;
	Mutex_Lock(&inode_lock);
	int rc = Allocate_Inode(newInodeNo);
	
	int blockNo = (*newInodeNo) / (BLOCK_SIZE /(int)sizeof(Inode)) + disk_superblock.firstInodeBlock;
	
	int indexInBlock = (*newInodeNo) % (BLOCK_SIZE /(int)sizeof(Inode));
	Inode *buf;
	rc = rc | Get_Into_Cache(blockNo, (char**)&buf);
	
	memcpy((void*)&buf[indexInBlock], (void*) &newInode,(int)sizeof(Inode));
	rc = rc | Set_Dirty(blockNo);
	rc = rc | Unfix_From_Cache(blockNo);
	rc = rc | Flush_Cache_Block(blockNo);
	Mutex_Unlock(&inode_lock);
	return rc;
}

// Gets a page into cache, or if it is there, icreases refcount
int Get_Inode_Into_Cache(int inodeNo, Inode** inode_buf)
{
	CacheInode *cache_inode;
	Mutex_Lock(&inode_cache_lock);	   
	int rc = Get_From_Hash_Table(&(inode_manager.cache_hash), inodeNo, (void**) &cache_inode);
	   
	if(rc == 0)
	{
		if(cache_inode->ref_count == 0) // In the free list
		{
			Unlink_Inode_From_Free(cache_inode);
			Link_Inode_To_Cache(cache_inode);
		}
		else // Else bring it to head
		{
			Unlink_Inode_From_Cache(cache_inode);
			Link_Inode_To_Cache(cache_inode);
		}
			
		// Already exists
		cache_inode->ref_count++;
		(*inode_buf) = &(cache_inode->inode);
	}
	else
	{
		   
		// Need to fetch from disk
		CacheInode *rep_page;
		if(inode_manager.current_cache_size < DISK_CACHE_SIZE) // Cache is still small
		{
		   
			// Allocate new cache page in the free list
			rep_page = (CacheInode*) Malloc(sizeof(CacheInode));
			rep_page->ref_count = 0;
			rep_page->dirty = 0;
			rep_page->inodeNo = -1; // Not a valid block
			Link_Inode_To_Free(rep_page);
			inode_manager.current_cache_size++;
		}
		else // Cache size is max
		{
			// Find replacement, write back if dirty, and load new page
		
			rc = Find_Inode_Replacement(&rep_page); // Gets replacement
			if(rc)	goto cleanAndReturn;
			rc = Write_Inode_If_Dirty(rep_page); // Cleans page
			if(rc)	goto cleanAndReturn;// Write fail
			int rc = Remove_From_Hash_Table(&(inode_manager.cache_hash), rep_page->inodeNo);
			if(rc)	goto cleanAndReturn; // Not in hash. should not occur
		}
		   
		// Read actual contents
		rc = Read_Inode_From_Disk(&(rep_page->inode), inodeNo);
		if(rc)	goto cleanAndReturn; // Read failed
		   
		// Add new entry to hash
		rc = Add_To_Hash_Table(&(inode_manager.cache_hash), inodeNo, (void**) rep_page);
		if(rc)	goto cleanAndReturn;
		   
		rep_page->inodeNo = inodeNo; // Set block number
		Unlink_Inode_From_Free(rep_page); // Remove from free
		Link_Inode_To_Cache(rep_page); // Add to main cache
		rep_page->ref_count = 1; // make refcount 1
		(*inode_buf) = &(rep_page->inode);
	}
	cleanAndReturn:
	Mutex_Unlock(&inode_cache_lock);
	return rc;
}

int Unfix_Inode_From_Cache(int inodeNo)
{
	Mutex_Lock(&inode_cache_lock);
	CacheInode* page;
	int rc = Get_From_Hash_Table((&(inode_manager.cache_hash)), inodeNo, (void**)(&page));
	if(rc) return rc;
	if(page->ref_count == 0)
	{
		return -1; // Cannot unfix
	}
	page->ref_count--;
	if(page->ref_count == 0)
	{
		Unlink_Inode_From_Cache(page);
		Link_Inode_To_Free(page);
	}
	Mutex_Unlock(&inode_cache_lock);
	return 0;
}


// Set page corresponding to inodeNo to dirty
int Set_Inode_Dirty(int inodeNo)
{
	Mutex_Lock(&inode_cache_lock);
	CacheInode* page;
	int rc = Get_From_Hash_Table(&(inode_manager.cache_hash), inodeNo, (void**)(&page));
	if(rc)
	{
		 return rc;
	}
	page->dirty = 1;
	Mutex_Unlock(&inode_cache_lock);
	return 0;
}

// Flush cache and make all pages clean
int Flush_Inode_Cache()
{	
	Mutex_Lock(&inode_cache_lock);
	int i;
	CacheInode* cur = inode_manager.first_inode;
	int rc = 0;
	while(cur != NULL)
	{
		rc = rc | Write_Inode_If_Dirty(cur);
		cur = cur->next;
	}
	cur = inode_manager.first_free_inode;
	while(cur != NULL)
	{
		rc = rc | Write_Inode_If_Dirty(cur);
		cur = cur->next;
	}
	Mutex_Unlock(&inode_cache_lock);
	return rc;
}

// Flush single page
int Flush_Inode_Cache_Item(int inodeNo)
{
	Mutex_Lock(&inode_cache_lock);
	CacheInode* page;
	int rc = Get_From_Hash_Table(&(inode_manager.cache_hash), inodeNo, (void**)&page);
	if(rc) return 0;
	Mutex_Unlock(&inode_cache_lock);
	return Write_Inode_If_Dirty(page);
}
	
// Finds a replacement from the free list
int Find_Inode_Replacement(CacheInode **rep_page)
{
	// No free page available
	if(inode_manager.last_free_inode == NULL) return -1;
	
	// Set cur 
	*rep_page = inode_manager.last_free_inode;
	return 0;
}

// Writes a page if it is dirty
int Write_Inode_If_Dirty(CacheInode *rep_page)
{
	int rc;
	if(rep_page->dirty)
	{
		rc = Write_Inode_To_Disk(&(rep_page->inode), rep_page->inodeNo);
		if(rc) return rc; // FATAL
		rep_page->dirty = 0; // Set to clean
	}
	return 0;
}

// Adds page to the front of free list
void Link_Inode_To_Free(CacheInode* page)
{
	page->next = inode_manager.first_free_inode;
	if(inode_manager.first_free_inode != NULL)
	{
		inode_manager.first_free_inode->prev = page;
	}
	inode_manager.first_free_inode = page;
	page->prev = NULL;
}

// Adds page to the front of cache list
void Link_Inode_To_Cache(CacheInode* page)
{
	page->next = inode_manager.first_inode;
	if(inode_manager.first_inode != NULL)
	{
		inode_manager.first_inode->prev = page;
	}
	inode_manager.first_inode = page;
	page->prev = NULL;
}

// Removes page from arbit position in the free list
void Unlink_Inode_From_Free(CacheInode* page)
{
	// Three cases, at the head, at the tail and in between
	if(page->prev == NULL) // at head
	{
		inode_manager.first_free_inode = page->next;
		if(page->next != NULL)
		{
			page->next->prev = NULL;
		}
		page->next = NULL;
	}
	else if(page->next == NULL) // at end
	{
		inode_manager.last_free_inode = page->prev;
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
void Unlink_Inode_From_Cache(CacheInode* page)
{
	// Three cases, at the head, at the tail and in between
	if(page->prev == NULL) // at head
	{
		inode_manager.first_inode = page->next;
		if(page->next != NULL)
		{
			page->next->prev = NULL;
		}
		page->next = NULL;
	}
	else if(page->next == NULL) // at end
	{
		inode_manager.last_inode = page->prev;
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

int Read_Inode_From_Disk(Inode *inode, int inodeNo) {
	int inodesPerBlock = BLOCK_SIZE /(int)sizeof(Inode);
	int blockNo = disk_superblock.firstInodeBlock + (inodeNo / inodesPerBlock);
	char *block;
	int rc = Get_Into_Cache(blockNo, &block);
	if (!rc) {
		int inodeStart = (sizeof(Inode) * (inodeNo % inodesPerBlock)) /(int)sizeof(char);
		memcpy((void*) inode, (void*) &block[inodeStart],(int)sizeof(Inode));
	}
	Unfix_From_Cache(blockNo);
	return rc;
}

int Write_Inode_To_Disk(Inode *inode, int inodeNo) {
	int inodesPerBlock = BLOCK_SIZE /(int)sizeof(Inode);
	int blockNo = disk_superblock.firstInodeBlock + (inodeNo / inodesPerBlock);
	char *block;
	int rc = Get_Into_Cache(blockNo, &block);
	if (!rc) {
		int inodeStart = (sizeof(Inode) * (inodeNo % inodesPerBlock)) /(int)sizeof(char);
		memcpy((void*) &block[inodeStart], (void*) inode,(int)sizeof(Inode));
	}
	Set_Dirty(blockNo);
	Unfix_From_Cache(blockNo);
	return rc;
}



int Allocate_Upto(int inodenum, int allocate_size)
{
	Inode* inode;
	int rc = Get_Inode_Into_Cache(inodenum, &inode);
	if(rc) return rc;
	// inode should be fixed in the cache
	if(allocate_size <= (inode->meta_data).file_size)
	{
		Unfix_Inode_From_Cache(inodenum);
		return -1;
	}
	int final_num_blocks = ((allocate_size - 1) / BLOCK_SIZE) + 1; // Final active number of blocks
	int current_num_blocks = (((inode->meta_data).file_size - 1) / BLOCK_SIZE) + 1; // Currently active number of blocks
	int orig_num_blocks = current_num_blocks; // Originally active number of blocks
	
	rc = 0;
	// Base ptr allocation
	for(; current_num_blocks < final_num_blocks &&  current_num_blocks < INODE_BASE_SIZE; ++current_num_blocks)
	{
		int alloc_block;
		rc = rc | Allocate_Block(&alloc_block);
		inode->entries[current_num_blocks] = alloc_block;
	}
	
	current_num_blocks -= INODE_BASE_SIZE;
	final_num_blocks -= INODE_BASE_SIZE;
	
	if(final_num_blocks <= 0) {
		inode->meta_data.file_size = allocate_size;
		Set_Inode_Dirty(inodenum);
		rc = Unfix_Inode_From_Cache(inodenum);
		return rc; // Allocation done
	}
	
	
	// First level allocation
	if(orig_num_blocks <= INODE_BASE_SIZE)
	{
		rc = rc | Allocate_Block(&(inode->s_nest_ptr));
	}
	int *buf;
	
	for(; current_num_blocks < final_num_blocks && current_num_blocks < BLOCK_SIZE /(int)sizeof(int); current_num_blocks++)
	{
		rc = rc | Get_Into_Cache(inode->s_nest_ptr, (char**)(&buf));
		rc = rc | Allocate_Block(&(buf[current_num_blocks]));
		rc = rc | Set_Dirty(inode->s_nest_ptr);
		rc = rc | Unfix_From_Cache(inode->s_nest_ptr);
	}
	
	current_num_blocks -= BLOCK_SIZE /(int)sizeof(int);
	final_num_blocks -= BLOCK_SIZE /(int)sizeof(int);
	
	if(final_num_blocks <= 0) {
		inode->meta_data.file_size = allocate_size;
		Set_Inode_Dirty(inodenum);
		rc = Unfix_Inode_From_Cache(inodenum);
		return rc; // Allocation done
	}
	
	// Second level allocation
	if(orig_num_blocks <= INODE_BASE_SIZE + BLOCK_SIZE /(int)sizeof(int))
	{
		rc = rc | Allocate_Block(&(inode->d_nest_ptr));
	}
	
	rc = rc | Get_Into_Cache(inode->d_nest_ptr, (char**) (&buf));
	
	
	// Go over all entries in the first indirect page pointed to by d_nest_ptr
	int i;
	for(i = 0; i < BLOCK_SIZE /(int)sizeof(int); ++i)
	{
		if(orig_num_blocks <= INODE_BASE_SIZE + (i + 1) * BLOCK_SIZE /(int)sizeof(int))
		{
			rc = rc | Allocate_Block(&(buf[i]));
		}
		int *buf2;
		
		for(; current_num_blocks < final_num_blocks && current_num_blocks < BLOCK_SIZE /(int)sizeof(int); current_num_blocks++)
		{
			rc = rc | Get_Into_Cache(buf[i], (char**)(&buf2));
			rc = rc | Allocate_Block(&(buf2[current_num_blocks]));
			rc = rc | Set_Dirty(buf[i]);
			rc = rc | Unfix_From_Cache(buf[i]);
		}
		
		
		current_num_blocks -= BLOCK_SIZE /(int)sizeof(int);
		final_num_blocks -= BLOCK_SIZE /(int)sizeof(int);
		
		if(final_num_blocks <= 0)
		{
			rc = rc | Unfix_From_Cache(inode->d_nest_ptr);
			inode->meta_data.file_size = allocate_size;
			Set_Inode_Dirty(inodenum);
			Unfix_Inode_From_Cache(inodenum);
			return rc; // Allocation done
		}
	}
	inode->meta_data.file_size = allocate_size;
	Set_Inode_Dirty(inodenum);
	rc = Unfix_Inode_From_Cache(inodenum);
	return 0;
}

int Get_Block_For_Byte_Address(Inode* inode, int byte_address, int* blocknum)
{
	if(byte_address > (inode->meta_data).file_size) return -1;
	
	int block_num = (byte_address / BLOCK_SIZE);
	if(block_num < INODE_BASE_SIZE) 
	{
		*blocknum = inode->entries[block_num];
		return 0;
	}
	block_num -= INODE_BASE_SIZE;
	if(block_num < BLOCK_SIZE /(int)sizeof(int))
	{
		int *buf, rc;
		rc = Get_Into_Cache(inode->s_nest_ptr, (char**)&buf);
		*blocknum = buf[block_num];
		rc = rc | Unfix_From_Cache(inode->s_nest_ptr);
		return rc;
	}
	
	block_num -= BLOCK_SIZE /(int)sizeof(int);
	
	int first_level_index = block_num / (BLOCK_SIZE /(int)sizeof(int));
	int second_level_index = block_num % (BLOCK_SIZE /(int)sizeof(int));
	int *buf;
	int rc = Get_Into_Cache(inode->d_nest_ptr, (char**)&buf);
	int sec_page = buf[first_level_index];
	rc = rc | Unfix_From_Cache(inode->d_nest_ptr);
	rc = rc | Get_Into_Cache(sec_page, (char**)&buf);
	*blocknum = buf[second_level_index];
	rc = rc | Unfix_From_Cache(sec_page);
	return rc;
	
}

int Truncate_From(int inodenum, int truncate_size)
{
	
	Inode* inode;
	int rc = Get_Inode_Into_Cache(inodenum, &inode);
	if(rc) return rc;
	// inode should be fixed in the cache
	if(truncate_size >= (inode->meta_data).file_size)
	{
		Unfix_Inode_From_Cache(inodenum);
		return -1;
	}
	int final_num_blocks = (((inode->meta_data).file_size - 1) / BLOCK_SIZE) + 1; // Final active number of blocks
	int current_num_blocks = ((truncate_size - 1) / BLOCK_SIZE) + 1; // Currently active number of blocks
	int orig_num_blocks = current_num_blocks; // Originally active number of blocks
	
	rc = 0;
	// Base ptr allocation
	for(; current_num_blocks < final_num_blocks &&  current_num_blocks < INODE_BASE_SIZE; ++current_num_blocks)
	{
		rc = rc | Free_Block(inode->entries[current_num_blocks]);
	}
	
	current_num_blocks -= INODE_BASE_SIZE;
	final_num_blocks -= INODE_BASE_SIZE;
	
	if(final_num_blocks <= 0) {
		inode->meta_data.file_size = truncate_size;
		Set_Inode_Dirty(inodenum);
		Unfix_Inode_From_Cache(inodenum);
		return rc; // Allocation done
	}
	
	
	// First level allocation
	
	int *buf;
	
	for(; current_num_blocks < final_num_blocks && current_num_blocks < BLOCK_SIZE /(int)sizeof(int); current_num_blocks++)
	{
		rc = rc | Get_Into_Cache(inode->s_nest_ptr, (char**)(&buf));
		rc = rc | Free_Block(buf[current_num_blocks]);
		rc = rc | Unfix_From_Cache(inode->s_nest_ptr);
	}
	
	if(orig_num_blocks <= INODE_BASE_SIZE)
	{
		rc = rc | Free_Block((inode->s_nest_ptr));
	}
	
	current_num_blocks -= BLOCK_SIZE /(int)sizeof(int);
	final_num_blocks -= BLOCK_SIZE /(int)sizeof(int);
	
	if(final_num_blocks <= 0) {
		inode->meta_data.file_size = truncate_size;
		Set_Inode_Dirty(inodenum);
		Unfix_Inode_From_Cache(inodenum);
		return rc; // Freeing done
	}
	
	// Second level allocation
	
	
	rc = rc | Get_Into_Cache(inode->d_nest_ptr, (char**) (&buf));
	
	
	// Go over all entries in the first indirect page pointed to by d_nest_ptr
	int i;
	for(i = 0; i < BLOCK_SIZE /(int)sizeof(int); ++i)
	{
		
		int *buf2;
		
		for(; current_num_blocks < final_num_blocks && current_num_blocks < BLOCK_SIZE /(int)sizeof(int); current_num_blocks++)
		{
			rc = rc | Get_Into_Cache(buf[i], (char**)(&buf2));
			rc = rc | Free_Block((buf2[current_num_blocks]));
			rc = rc | Unfix_From_Cache(buf[i]);
		}
		
		
		current_num_blocks -= BLOCK_SIZE /(int)sizeof(int);
		final_num_blocks -= BLOCK_SIZE /(int)sizeof(int);
		
		if(orig_num_blocks <= INODE_BASE_SIZE + (i + 1) * BLOCK_SIZE /(int)sizeof(int))
		{
			rc = rc | Free_Block((buf[i]));
		}
		
		if(final_num_blocks <= 0)
		{
			rc = rc | Unfix_From_Cache(inode->d_nest_ptr);
			if(orig_num_blocks	 <= INODE_BASE_SIZE + BLOCK_SIZE /(int)sizeof(int))
			{
				rc = rc | Free_Block((inode->d_nest_ptr));
			}
			inode->meta_data.file_size = truncate_size;
			Set_Inode_Dirty(inodenum);
			Unfix_Inode_From_Cache(inodenum);
			return rc; // Allocation done
		}
	}
	inode->meta_data.file_size = truncate_size;
	Set_Inode_Dirty(inodenum);
	Unfix_Inode_From_Cache(inodenum);
	return 0;
}
