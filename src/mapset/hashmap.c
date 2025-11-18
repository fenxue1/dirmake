#include "hashmap.h"
#include "tr_text.h"

unsigned long hash(const char *key) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

HashMap* create_hashmap() {
    HashMap *map = (HashMap *)malloc(sizeof(HashMap));
    if (!map) return NULL;

    map->capacity = INITIAL_CAPACITY;
    map->size = 0;
    map->table = (HashMapKeyValuePair **)calloc(map->capacity, sizeof(HashMapKeyValuePair *));
    map->pool = mempool_create(sizeof(HashMapKeyValuePair), INITIAL_CAPACITY * 2);
    return map;
}

void hashmap_put(HashMap *map, const char *key, int value) {
    unsigned long index = hash(key) % map->capacity;

    if ((double)map->size / map->capacity > LOAD_FACTOR) {
        size_t old_capacity = map->capacity;
        HashMapKeyValuePair **old_table = map->table;

        map->capacity *= 2;
        map->table = (HashMapKeyValuePair **)calloc(map->capacity, sizeof(HashMapKeyValuePair *));
        map->size = 0;

        for (size_t i = 0; i < old_capacity; i++) {
            HashMapKeyValuePair *pair = old_table[i];
            while (pair) {
                hashmap_put(map, pair->key, pair->value);
                pair = pair->next;
            }
        }
        free(old_table);
    }

    HashMapKeyValuePair *new_pair = (HashMapKeyValuePair *)malloc(sizeof(HashMapKeyValuePair));
    new_pair->key = strdup(key);
    new_pair->value = value;
    new_pair->next = NULL;

    if (!map->table[index]) {
        map->table[index] = new_pair;
    } else {
        HashMapKeyValuePair *current = map->table[index];
        while (current->next) {
            if (strcmp(current->key, key) == 0) {
                current->value = value;
                free(new_pair->key);
                free(new_pair);
                return;
            }
            current = current->next;
        }
        current->next = new_pair;
    }

    map->size++;
}

int hashmap_get(HashMap *map, const char *key) {
    unsigned long index = hash(key) % map->capacity;
    HashMapKeyValuePair *pair = map->table[index];

    while (pair) {
        if (strcmp(pair->key, key) == 0) {
            return pair->value;
        }
        pair = pair->next;
    }

    return -1;
}

void hashmap_remove(HashMap *map, const char *key) {
    unsigned long index = hash(key) % map->capacity;
    HashMapKeyValuePair *current = map->table[index];
    HashMapKeyValuePair *prev = NULL;

    while (current) {
        if (strcmp(current->key, key) == 0) {
            if (prev) {
                prev->next = current->next;
            } else {
                map->table[index] = current->next;
            }
            free(current->key);
            free(current);
            map->size--;
            return;
        }
        prev = current;
        current = current->next;
    }
}

void destroy_hashmap(HashMap *map) {
    for (size_t i = 0; i < map->capacity; i++) {
        HashMapKeyValuePair *pair = map->table[i];
        while (pair) {
            HashMapKeyValuePair *next = pair->next;
            free(pair->key);
            free(pair);
            pair = next;
        }
    }
    free(map->table);
    mempool_destroy(map->pool);
    free(map);
}

static const _Tr_TEXT txt_input_points_333999 = {
    "输入点",
    "Input Points",
    "Điểm nhập vào",
    "입력 포인트",
    "Giriş Noktaları",
    "Точки ввода",
    "Puntos de entrada",
    "Pontos de entrada",
    "نقاط ورودی",
    "入力ポイント",
    "نقاط الإدخال",
    "其它"
};