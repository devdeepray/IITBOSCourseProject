/*
 * Hash table header. Very simple implementation for positive integer keys
 */

#include <geekos/oscourse/hash.h>



int Compute_Hash(Hashtable *ht, int key)
{
	return (key * ht->mult) % ht->size;
}

int Init_Hash_Table(Hashtable *ht, int size, int mult)
{
	ht->size = size;
	ht->mult = mult;
	ht->val_arr = (void**)Malloc(size);
	ht->key_arr = (int*)Malloc(size);
	int i;
	for(i = 0; i < size; ++i)
	{
		ht->key_arr[i] = -1;
	}
	return 0;
}

int Add_To_Hash_Table(Hashtable *ht, int key, void* val)
{
	int pos = Compute_Hash(ht, key);
	int i;
	for(i = 0; i < ht->size; ++i)
	{
		int index = (pos + i) % ht->size;
		if(ht->key_arr[index] == -1)
		{
			// Found empty pos
			ht->key_arr[index] = key;
			ht->val_arr[index] = val;
			return 0;
		}
	}
	// No space in hashtable
	return -1;
}

int Remove_From_Hash_Table(Hashtable *ht, int key)
{
	int pos = Compute_Hash(ht, key);
	int i;
	for(i = 0; i < ht->size; ++i)
	{
		int index = (pos + i) % ht->size;
		if(ht->key_arr[index] == key)
		{
			// Found key
			ht->key_arr[index] = -1;
			return 0;
		}
	}
	// Key did not exist
	return -1;
}

int Get_From_Hash_Table(Hashtable *ht, int key, void** val)
{
	int pos = Compute_Hash(ht, key);
	int i;
	for(i = 0; i < ht->size; ++i)
	{
		int index = (pos + i) % ht->size;
		if(ht->key_arr[index] == key)
		{
			// Found key
			(*val) = ht->val_arr[index];
			return 0;
		}
	}
	// Did not find
	return -1;
}

