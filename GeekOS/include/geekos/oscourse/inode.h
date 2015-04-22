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
#include <geekos/oscourse/hash.h>
#include <geekos/oscourse/blocks.h>
#include <geekos/string.h>

typedef struct InodeMetaData{

	char filename[MAX_FILE_NAME_LENGTH];
	   int group_id, owner_id, permissions, file_size;
} InodeMetaData;

typedef struct Inode{

	InodeMetaData meta_data; // File metadata	
	int entries[INODE_BASE_SIZE]; // Base ptrs
	int s_nest_ptr; // Single nested ptr 
	int d_nest_ptr; // Double nested ptr
} Inode;

typedef struct CacheInode{

	Inode inode;
	int inodeNo;
	int ref_count, dirty;
	struct CacheInode *prev, *next;
} CacheInode;


typedef struct InodeManager{
	int bitmap_start_block,i_node_start_block;
	int bitmap_number_of_blocks,i_node_number_of_blocks;
	
	
	int max_bitmap_size;

	CacheInode* first_free_inode;
	CacheInode* last_free_inode;
	CacheInode* first_inode;
	CacheInode* last_inode;
	
	int current_cache_size;
	Hashtable cache_hash;
	
} InodeManager;

// Cache functions
int Init_Inode_Cache();

int Allocate_Inode(int*);

int Free_Inode(int);

int Get_Inode_Into_Cache(int inodeNo, Inode** buf);

int Set_Inode_Dirty(int inodeNo);

int Unfix_Inode_From_Cache(int inodeNo);

int Flush_Inode_Cache();

int Free_Inode_Cache();

int Flush_Inode_Cache_Item(int inodeNo);

void Unlink_Inode_From_Cache(CacheInode*);

void Unlink_Inode_From_Free(CacheInode*);

void Link_Inode_To_Cache(CacheInode*);

void Link_Inode_To_Free(CacheInode*);

int Find_Inode_Replacement(CacheInode**);

int Write_Inode_If_Dirty(CacheInode*);

int Read_Inode_From_Disk(Inode*, int);

int Write_Inode_To_Disk(Inode*, int);

int Allocate_Upto(int, int);

int Get_Block_For_Byte_Address(Inode*, int, int*);

int Truncate_From(int, int);

int Create_New_Inode(InodeMetaData meta_data, int* newInodeNo);

int Init_Inode_Manager();

int Shut_Down_Inode_Manager();

#endif

