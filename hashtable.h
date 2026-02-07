#ifndef _HASHTABLEH_
#define _HASHTABLEH_

#include "memory.h"
#include "string.h"

#define TABLE_SIZE 32

typedef struct Entry {
	String key;
	void* value;
	struct Entry *next;
} Entry;

typedef struct HashTable {
	Entry *table[TABLE_SIZE];
	Arena *arena;
} HashTable;

HashTable *hashtable_create(Arena *a);
void hashtable_insert(HashTable *hashtable, String key, void *value);
Entry *hashtable_get(HashTable *hashtable, String key);
unsigned long djb2_hash(String key);

#endif
