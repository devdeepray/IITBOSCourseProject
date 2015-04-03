/* This is the disk cache manager. It reads stuff from the disk and 
 * implements a LRU cache replacement policy for the pages. 
 * 
 * Structures requred:
 * 1. The cache. Just an array of blocks, and some extra metadata per 
 * 		block
 * 2. Metadata of each page in the cache.
 * 3. Locking system so that page in use is not evicted.
 * 
 * Functions required:
 * 1. Get_Page(int block_num, char ** buf)
 * -> Makes buf point to the correct page, after loading/replacement
 * 		and locks the page.
 * -> Should give an error if same page is requested when locked.
 * 2. Set_Dirty(int block_num)
 * -> Set the dirty bit to true
 * 3. Release_Page(int block_num)
 * -> releases the page from memory, reducing memory usage
 * 
 * Available functions:
 * fsinit.h and virtualdisk.h
