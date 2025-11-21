#ifndef __HASHMAP_H_
#define __HASHMAP_H_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../mempool/mempool.h"
#define INITIAL_CAPACITY 16
#define LOAD_FACTOR 0.75

typedef struct HashMapKeyValuePair {
    char *key;
    int value;
    struct HashMapKeyValuePair *next;
} HashMapKeyValuePair;

typedef struct {
    size_t capacity;
    size_t size;
    HashMapKeyValuePair **table;
    MemoryPool *pool;
} HashMap;

unsigned long hash(const char *key);
HashMap* create_hashmap();
void hashmap_put(HashMap *map, const char *key, int value);
int hashmap_get(HashMap *map, const char *key);
void hashmap_remove(HashMap *map, const char *key);
void destroy_hashmap(HashMap *map);
#endif

