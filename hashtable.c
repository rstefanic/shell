#include "hashtable.h"
#include "memory.h"

HashTable *hashtable_create(Arena *a) {
	HashTable *hashtable = arena_alloc(a, sizeof(HashTable));
	return hashtable;
}

void hashtable_insert(HashTable *hashtable, Arena *a, String key, void *value) {
	size_t idx = djb2_hash(key) % TABLE_SIZE;
	Entry *entry = arena_alloc(a, sizeof(Entry));
	hashtable->table[idx] = entry;
	entry->key = key;
	entry->value = value;
}

Entry *hashtable_get(HashTable *hashtable, String key) {
	size_t hash = djb2_hash(key) % TABLE_SIZE;
	return hashtable->table[hash];
}

unsigned long djb2_hash(String key) {
	unsigned long hash = 5381; // magic starting value
	for (size_t i = 0; i < key.len; i++) {
		char c = key.value[i];
		hash = hash * 33 + c;
	}
	return hash;
}
