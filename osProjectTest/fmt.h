/*
 * File map table (aka inode) data structure and functions. 
 * Copyright (c) 2015, Group 10 CSE 2016
 *
 * All rights reserved.
 *
 * This code may not be redistributed without the permission of the copyright holders.
 */


#ifndef FMT_H
#define FMT_H

#include <geekos/oscourse/fsysdef.h>


typedef struct InodeMetaData{

	char filename[FILENAME_SIZE];
	int group_id, owner_id, permissions, file_size;
} InodeMetaData;

typedef struct Inode{

	InodeMetaData meta_data; // File metadata	
	int entries[INODE_SIZE]; // Base ptrs
	int s_nest_ptr; // Single nested ptr 
	int d_nest_ptr; // Double nested ptr
} Inode;

typedef struct InodeItem{

	int ref_count, dirty;
	Inode inode;
	InodeItem* prev, next;
} InodeItem;

typedef struct InodeManager{
	int bitmap_start_block,i_node_start_block;
	int bitmap_number_of_blocks,i_node_number_of_blocks;
	
	int max_cache_size;
	int cache_size;
	int max_bitmap_size;
	int start_i_node_block;

	InodeItem* cache_head;
	InodeItem* cache_tail;
} InodeManager;

InodeManager* i_node_manager;

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
void Format_Inode_Blocks();

#endif
