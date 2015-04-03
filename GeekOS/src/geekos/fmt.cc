#include <geekos/fmt.h>
#include <math.h>

/* HELPER FUNCTIONS */
int minimum(int a, int b){
	if(a < b)
		return a;
	return b;
}

//gets the block number for a <inode index>
int getBlockForIndex(int index){
	int number_of_i_node_per_block = BLOCK_SIZE/sizeof(Inode);
	return i_node_manager->start_i_node_block + index/number_of_i_node_per_block;
}

//gets the first empty inode position
int getFirstEmpty(char[] bitmap, int max_bitmap_size){
	//bitmap : 1 block
	//max_bitmap_size : max bitmaps allowed in that block(or overflow)

	int i,pos_in_char;
	for(i = 0 ; i < BLOCK_SIZE; i++){
		int current_location_max = max_bitmap_size - i*8;
		//Might have to use "unsigned int" conversion of char
		int power_of_2 = 0x80;
		for(pos_in_char = 0; pos_in_char < 8; pos_in_char++){
			if(current_location_max > pos_in_char && (bitmap[i] & power_of_2 == 0))
				return i*8 + pos_in_char;
			power_of_2 /= 2;
		}
		if(current_location_max <= 8)
			return -1;
	}
	return -1;
}

//assigns an Inode into the buffer
void writeInodeIntoBuffer(char[] buffer, int i_node_index; Inode* data){
	int i_node_index_in_buffer = i_node_index%(BUFFER_SIZE/sizeof(Inode));
	int start_location = i_node_index_in_buffer*sizeof(Inode);
	memcpy(&(buffer[start_location]),data,sizeof(Inode));
}

//loads an inode from the buffer
int loadInodeFromBuffer(char[] i_node_block_buffer,int new_i_node_index,Inode* i_node){
	int i_node_index_in_buffer = i_node_index%(BUFFER_SIZE/sizeof(Inode));
	int start_location = i_node_index_in_buffer*sizeof(Inode);
	memcpy(i_node,&(buffer[start_location]),sizeof(Inode));	
	return 0;
}

//initialises a new inode item
int initInodeItem(InodeItem* i_node_item,Inode* i_node, int i_node_index, int user_count){
	i_node_item->i_node = *i_node;
	i_node_item->i_node_index = i_node_index;
	i_node_item->prev = NULL;
	i_node_item->next = NULL;
	i_node_item->user_count = user_count;
	i_node_item->dirty = 0;
	return 0;
}

void removeInodeItem (InodeItem* i_node_item){
	if(i_node_item->prev != NULL){
		i_node_item->prev->next = i_node_item->next;
	}if(i_node_item->next != NULL){
		i_node_item->next->prev = i_node_item->prev;
	}
	if(i_node_item->dirty == 1)	
		writeBackIntoBlock(&i_node_item->i_node, i_node_item->i_node_index);
	// delete(i_node_item);
	i_node_manager->cache_size--;
}

int swapOutFromCache(InodeItem** i_node_item_pointer){
	//POSSIBLE BETTER CODE
	InodeItem* current_i_node_item = i_node_manager->cache;
	InodeItem* swappable = NULL;
	while(current_i_node_item != NULL){
		if(current_i_node_item->user_count == 0)
			swappable = current_i_node_item;
		current_i_node_item = current_i_node_item->next;
	}
	if(swappable == NULL)
		return 0;
	removeInodeItem(swappable);
	(*i_node_item_pointer) = swappable;
	return 1;
}

InodeItem* searchInCache(int i_node_index){
	InodeItem* current_i_node_item = i_node_manager->cache;
	while(current_i_node_item != NULL){
		if(current_i_node_item->i_node_index == i_node_index){
			return current_i_node_item;
		}
	}
	return NULL;
}
int releaseInCache(int i_node_index){
	InodeItem* current_i_node_item = i_node_manager->cache;
	while(current_i_node_item != NULL){
		if(current_i_node_item->i_node_index == i_node_index){
			if(current_i_node_item->user_count == 0)
				return -1;
			current_i_node_item->user_count--;
			return 1;
		}
	}
	return 0;
}

void addToCache(Inode* i_node, int i_node_index, int increment=0){
	if(i_node_manager->cache == NULL){
		InodeItem* i_node_item = (InodeItem*) malloc(sizeof(InodeItem));
		initInodeItem(i_node_item,i_node,i_node_index,increment);
		i_node_manager->cache = i_node_item;
		i_node_manager->cache_size++;
	}
	else if(i_node_manager->cache_size < i_node_manager->max_cache_size){
		InodeItem* i_node_item = (InodeItem*) malloc(sizeof(InodeItem));
		initInodeItem(i_node_item,i_node,i_node_index,increment);
		i_node_manager->cache->prev = i_node_item;
		i_node_item->next = i_node_manager->cache;
		i_node_manager->cache = i_node_item;
		cache_size++;
	}else{
		InodeItem* i_node_item;
		int swapped_something = swapOutFromCache(&i_node_item);
		if(swapped_something == 1){
			initInodeItem(i_node_item,i_node,i_node_index,increment);
			i_node_manager->cache->prev = i_node_item;
			i_node_item->next = i_node_manager->cache;
			i_node_manager->cache = i_node_item;
			cache_size++;
		}else{
			//make the thing wait
		}
	}
}

void writeBackIntoBlock(Inode* i_node, int i_node_index){
	char i_node_block_buffer[BLOCK_SIZE];
	//Fetch that Inode
	int i_node_block = getBlockForIndex(i_node_manager,i_node_index);
	Fegade_getBlock(i_node_block,&i_node_block_buffer);
	
	writeInodeIntoBuffer(i_node_block_buffer,new_i_node_index,i_node);
	//Write Back
	// Fegade_setDirtyBlock(i_node_block);
	Fegade_writeIntoBlock(i_node_block,i_node_block_buffer);
}

int getNextBlock(int *i_1,int *i_2,int *i_3){
	int &index_1 = *i_1;
	int &index_2 = *i_2;
	int &index_3 = *i_3;
	if(index_1 <= 11){
		index_1 = 12;	index_2 = -1;	index_3 = -1;
	}else if(index_1 == 12 && index_2 == (BLOCK_SIZE/sizeof(int))-1){
		index_1 = 13;	index_2 = -1; 	index_3 = -1;
	}else if(index_1 == 12){
		index_2++;
	}else if(index_1 == 13 && index_3 == (BLOCK_SIZE/sizeof(int))-1){
		index_2++; 	index_3 = -1;
	}else if(index_1 == 13){
		index_3++;
	}

}


int addBlockAt(Inode* i_node, int index_1, int index_2, int index_3, int block_address){
	int int_size = sizeof(int);
	int new_block_address;
	Fegade_allocateNewBlock(&new_block_address);

	if(index_2 == -1){
		i_node->entries[index_1] = new_block_address;
		return new_block_address;
	}else{
		int index = index_2; 
		if(index_1 == 12 || index_3 == -1)
			index = index_2;
		else if(index_1 == 13)
			index = index_3;

		char buffer[BLOCK_SIZE];
		Fegade_getBlock(block_address,buffer);
		memcpy(&buffer[index_2*int_size],&new_block_address,int_size);
		Fegade_writeIntoBlock(block_address,buffer);
		return new_block_address;
	}else{
		Fegade_releaseBlock(new_block_address);
		return -1;
	}
}

int getEntryFromBlock(int block_address,int index){
	char buffer[BLOCK_SIZE];
	Fegade_getBlock(block_address,buffer);
	int new_block_address = ((int) buffer)[index];
	// memcpy(&new_block_address,&buffer[index*sizeof(int)],sizeof(int));
	return new_block_address;
}

int getBlockAddressForIndex(Inode* i_node,int index_1,int index_2,int index_3){
	if(index_1 < 12)
		return i_node->entries[index_1];
	else if(index_1 == 12){
		return getEntryFromBlock(i_node->entries[index_1],index_2);
	}
	else if(index_1 == 13){
		int block_address = getEntryFromBlock(i_node->entries[index_1],index_2);
		return getEntryFromBlock(block_address,index_3);
	}
}


int getLocationOfByteAddress(int byte_address,int &index_1, int &index_2, int &index_3){
	int int_size = sizeof(int);
	int level_1_size = 12*BLOCK_SIZE;
	int level_2_size = (BLOCK_SIZE/int_size)*BLOCK_SIZE;
	int level_3_size = (BLOCK_SIZE/int_size)*(BLOCK_SIZE/int_size)*BLOCK_SIZE;
	
	if(byte_address < 0){
		return -1;
	}else if(byte_address < level_1_size){
		index_1 = byte_address/BLOCK_SIZE;
	}else if(byte_address < level_1_size + level_2_size){
		index_1 = 12;
		index_2 = (byte_address - level_1_size)/BLOCK_SIZE;
	}else if(byte_address < level_1_size + level_2_size + level_3_size){
		index_1 = 13;
		index_2 = (byte_address - level_1_size - level_2_size)/level_2_size;
		index_3 = (byte_address - level_1_size - level_2_size - index_2*level_2_size)/BLOCK_SIZE;
	}else{
		return -1;
	}
	return 1;
}


/* API */
int Init_Inode_Manager(int bitmap_start_block, int bitmap_number_of_blocks,
	int i_node_start_block, int i_node_number_of_blocks){

	//CHANGE FROM HERE
	i_node_manager->bitmap_start_block = bitmap_start_block;
	i_node_manager->bitmap_number_of_blocks = bitmap_number_of_blocks;

	i_node_manager->i_node_start_block = i_node_start_block;
	i_node_manager->i_node_number_of_blocks = i_node_number_of_blocks;
	//CHANGE TILL HERE

	i_node_manager->max_cache_size = (int) CACHE_SIZE/sizeof(InodeItem);
	i_node_manager->cache_size = 0;
	i_node_manager->max_bitmap_size = 
		minimum(i_node_manager->i_node_number_of_blocks/sizeof(Inode),
			i_node_manager->bitmap_number_of_blocks*BLOCK_SIZE*8);
	
	i_node_manager->cache = NULL;
	return 1;
}

void Format_Inode_Blocks(){
	;//TODO: If Fegade already formats the kernel space this is not necessary
}

int Create_New_Inode(MetaData meta_data){
	//Fetch the array of bitmaps
	int current_block = i_node_manager->bitmap_start_block;
	int bitmap_block_count = 1;
	int new_i_node_index = -1;
	int max_bitmap_size = i_node_manager->max_bitmap_size;
	char bitmap_block_buffer[BLOCK_SIZE];

	//CLEANER CODE POSSIBLE
	while(new_i_node_index == -1 && bitmap_block_count <= i_node_manager->bitmap_number_of_blocks){
		Fegade_getBlock(&bitmap_block_buffer);
		if(max_bitmap_size > BLOCK_SIZE*8)
			new_i_node_index = getFirstEmpty(bitmap_block_buffer,BLOCK_SIZE*8);
		else
			new_i_node_index = getFirstEmpty(bitmap_block_buffer,max_bitmap_size);
		max_bitmap_size -= BLOCK_SIZE;
	}
	if(new_i_node_index == -1)
		return -1;

	//Create the I_Node
	Inode i_node;
	i_node.meta_data = meta_data;
	writeBackIntoBlock(&i_node,new_i_node_index);
	//Add to cache
	addToCache(i_node);

	//Return
	return new_i_node_index;
}


int Load_Inode(int i_node_index, Inode** i_node){
	InodeItem* searched = searchInCache(i_node_index);
	if(searched != NULL){
		searched->user_count++;
		(*i_node) = &(searched->i_node);
		return 1;
	}else{
		//Fetch and Initialise
		char i_node_block_buffer[BLOCK_SIZE];
		int i_node_block = getBlockForIndex(i_node_index);
		Fegade_getBlock(i_node_block,&i_node_block_buffer);
		Inode new_i_node;
		loadInodeFromBuffer(i_node_block_buffer,i_node_index,&new_i_node);
		addToCache(new_i_node,i_node_index,1);
		(*i_node) = new_i_node;
		return 1;
	}
}

int Release_Inode(int i_node_index){
	return releaseInCache(i_node_index);
}

int Allocate_Upto(Inode* i_node, int allocate_size){
	if(allocate_size > i_node->meta_data.file_size){
		i_node->meta_data.file_size = allocate_size;
	}

	if((int)(allocate_size/BLOCK_SIZE) <= (int)(i_node->meta_data.file_size/BLOCK_SIZE)){
		return 1;
	}

	int start_index_1 = -1, start_index_2 = -1, start_index_3 = -1;
	getLocationOfByteAddress(i_node->meta_data.file_size-1,
		start_index_1,start_index_2,start_index_3);

	int index_1 = -1,index_2 = -1,index_3 = -1;
	int rc = getLocationOfByteAddress(allocate_size-1,index_1,index_2,index_3);
	if(rc == -1)
		return rc;
	
	int next_block;
	int loop_condition = true;
	while(loop_condition){
		loop_condition = !(start_index_1 == index_1 && start_index_2 == index_2 && start_index_3 == index_3);
		getNextBlock(&start_index_1,&start_index_2,&start_index_3);
		next_block = addBlockAt(i_node,start_index_1,start_index_2,start_index_3,next_block);		
	}
	return 1;
}

int Get_Block_For_Byte_Address(Inode* i_node, int byte_address){
	int index_1 = -1,index_2 = -1,index_3 = -1;
	getLocationOfByteAddress(byte_address,index_1,index_2,index_3);		
	return getBlockAddressForIndex(i_node,index_1,index_2,index_3);
}