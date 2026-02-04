#include <assert.h>
#include <stdbool.h>

#include "hashtable.h"
#include "memory.h"
#include "string.h"

HashTable *hashtable_create(Arena *a) {
	HashTable *hashtable = arena_alloc(a, sizeof(HashTable));
	return hashtable;
}

void hashtable_insert(HashTable *hashtable, Arena *a, String key, void *value) {
	size_t idx = djb2_hash(key) % TABLE_SIZE;

	// If the key already exists, then overwrite its value.
	Entry *curr = hashtable->table[idx];
	while (curr != NULL) {
		if (string_compare(&curr->key, &key)) {
			curr->value = value;
			return;
		}
		curr = curr->next;
	}

	// Otherwise create a new entry.
	Entry *entry = arena_alloc(a, sizeof(Entry));
	entry->key = key;
	entry->value = value;
	entry->next = hashtable->table[idx];
	hashtable->table[idx] = entry;
}

Entry *hashtable_get(HashTable *hashtable, String key) {
	size_t hash = djb2_hash(key) % TABLE_SIZE;
	Entry *curr = hashtable->table[hash];
	while (curr != NULL) {
		if (string_compare(&curr->key, &key)) {
			return curr;
		}
		curr = curr->next;
	}

	return NULL;
}

unsigned long djb2_hash(String key) {
	unsigned long hash = 5381; // magic starting value
	for (size_t i = 0; i < key.len; i++) {
		char c = key.value[i];
		hash = hash * 33 + c;
	}
	return hash;
}
