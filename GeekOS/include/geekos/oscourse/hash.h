/*
 * Hash table header. Very simple implementation
 */
 
#ifndef _HASH_H_
#define _HASH_H_
typedef struct Hashtable_s
{
	int size;
	int mult;
	void **val_arr;
	int *key_arr;
} Hashtable;

int Init_Hash_Table(Hashtable *ht, int size, int mult);

int Clear_Hash_Table(Hashtable *ht);

int Add_To_Hash_Table(Hashtable *ht, int key, void* val);

int Remove_From_Hash_Table(Hashtable *ht, int key);

int Get_From_Hash_Table(Hashtable *ht, int key, void** val);

#endif
