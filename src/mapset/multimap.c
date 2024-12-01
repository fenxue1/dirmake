/*
 * @Author: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @Date: 2024-11-27 01:05:01
 * @LastEditors: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @LastEditTime: 2024-11-27 07:40:50
 * @FilePath: \test_cmake\src\mapset\multimap.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "mapset.h"

// 创建一个新的 MultiMap
MultiMap* create_multimap() {
    MultiMap* map = (MultiMap*)malloc(sizeof(MultiMap));
    if (!map) return NULL;
    map->head = NULL;
    return map;
}


// 插入键值对
void multimap_insert(MultiMap* map, const char* key, int value) {
    if (!map || !key) return;

    // 查找键是否已经存在
    KeyValuePair* current = map->head;
    while (current) {
        if (strcmp(current->key, key) == 0) {
            // 如果键存在，添加值到值列表
            ValueNode* new_value = (ValueNode*)malloc(sizeof(ValueNode));
            if (!new_value) return;
            new_value->value = value;
            new_value->next = current->values;
            current->values = new_value;
            return;
        }
        current = current->next;
    }

    // 如果键不存在，创建新的键值对
    KeyValuePair* new_pair = malloc(sizeof(KeyValuePair));
    if (!new_pair) return;
    new_pair->key = strdup(key);
    if (!new_pair->key) {
        free(new_pair);
        return;
    }
    new_pair->values = (ValueNode*)malloc(sizeof(ValueNode));
    if (!new_pair->values) {
        free(new_pair->key);
        free(new_pair);
        return;
    }
    new_pair->values->value = value;
    new_pair->values->next = NULL;
    new_pair->next = map->head;
    map->head = new_pair;
}


ValueNode* mapmultimap_find(MultiMap* map, const char* key) {
    if (!map || !key) return NULL;

    KeyValuePair* current = map->head;
    while (current) {
        if (strcmp(current->key, key) == 0) {
            return current->values;
        }
        current = current->next;
    }
    return NULL;
}



void mapmultimap_remove(MultiMap* map, const char* key) {
    if (!map || !key) return;

    KeyValuePair* current = map->head;
    KeyValuePair* prev = NULL;
    while (current) {
        if (strcmp(current->key, key) == 0) {
            if (prev) {
                prev->next = current->next;
            } else {
                map->head = current->next;
            }
            // 释放值列表
            ValueNode* value_current = current->values;
            while (value_current) {
                ValueNode* value_next = value_current->next;
                free(value_current);
                value_current = value_next;
            }
            free(current->key);
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}


void destroy_mapmultimap(MultiMap* map) {
    if (!map) return;

    KeyValuePair* current = map->head;
    while (current) {
        KeyValuePair* next = current->next;
        // 释放值列表
        ValueNode* value_current = current->values;
        while (value_current) {
            ValueNode* value_next = value_current->next;
            free(value_current);
            value_current = value_next;
        }
        free(current->key);
        free(current);
        current = next;
    }
    free(map);
}

void test_mapmultimap() {
    MultiMap* map = create_multimap();
    multimap_insert(map, "key1", 10);
    multimap_insert(map, "key1", 20);
    multimap_insert(map, "key2", 30);

    ValueNode* values = mapmultimap_find(map, "key1");
    printf("Values for key1: ");
    while (values) {
        printf("%d ", values->value);
        values = values->next;
    }
    printf("\n");

    mapmultimap_remove(map, "key1");

    destroy_mapmultimap(map);
}