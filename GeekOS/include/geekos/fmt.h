#ifndef FMT_H
#define FMT_H

#define CACHE_SIZE 4096
#define BLOCK_SIZE 4096
#define FILENAME_SIZE 20
#define INODE_SIZE 14
//[Super Block][[Inode BITMAP][i+1.......,j]][..........]

//[...12 directly to disk blocks...]
//[1 points to a disk block full of pointers]
//[1 points to a disk block full of pointers to block of pointers]


typedef struct MetaData{
	// int file_descriptor;
	// char filename[FILENAME_SIZE];
	int group_id, permissions, file_size;
} MetaData;

typedef struct Inode{
	MetaData meta_data;	
	int entries[INODE_SIZE];
} Inode;

typedef struct InodeItem{
	int i_node_index;
	int user_count, dirty;
	Inode i_node;
	InodeItem* prev, next;
} InodeItem;

typedef struct InodeManager{
	int bitmap_start_block,i_node_start_block;
	int bitmap_number_of_blocks,i_node_number_of_blocks;
	
	int max_cache_size;
	int cache_size;
	int max_bitmap_size;
	int start_i_node_block;

	InodeItem* cache;
} InodeManager;

InodeManager* i_node_manager;

//Helper
int minimum(int a, int b);
InodeItem* createInodeItem(Inode* i_node, int i_node_index, int user_count=0);

//Manager Functions
int getBlockForIndex(int index);
int getFirstEmpty(char[] bitmap, int max_bitmap_size);
void removeInodeItem (InodeItem* i_node_item);
Inode* loadInodeFromBuffer(char[] i_node_block_buffer,int new_i_node_index);

//Cache Functions
int swapOutFromCache();
InodeItem* searchInCache(int i_node_index);
int releaseInCache(int i_node_index);
void addToCache(Inode* i_node, int i_node_index, int increment=0);

//Write Functions
void writeInodeIntoBuffer(char[] buffer, int i_node_index; Inode* data);
void writeBack(Inode* i_node, int i_node_index, char i_node_block_buffer[]);
void writeBackIntoBlock(Inode* i_node, int i_node_index);

//Functions for the I_Node entries
int getLocationOfByteAddress(int byte_address,int &index_1, int &index_2, int &index_3);
int getNextBlock(int &index_1,int &index_2,int &index_3);
int addBlockAt(Inode* i_node, int index_1, int index_2, int index_3, int block_address=0);
int getEntryFromBlock(int block_address,int index);
int getBlockAddressForIndex(Inode* i_node,int index_1,int index_2,int index_3);

//API
int Init_Inode_Manager(int bitmap_start_block, int bitmap_number_of_blocks,
	int i_node_start_block, int i_node_number_of_blocks);
int Create_New_Inode(MetaData meta_data);
int Load_Inode(int i_node_index, Inode** i_node);
int Release_Inode(int i_node_index);
int Allocate_Upto(Inode* i_node, int allocate_size);
int Get_Block_For_Byte_Address(Inode* i_node, int byte_address);
void Format_Inode_Blocks();

#endif