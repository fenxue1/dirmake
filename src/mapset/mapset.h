/*
 * @Author: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @Date: 2024-11-22 00:38:43
 * @LastEditors: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @LastEditTime: 2024-11-27 06:59:07
 * @FilePath: \test_cmake\src\mapset\mapset.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __MAPSET_H_
#define __MAPSET_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// 定义用于存储值的节点
typedef struct ValueNode {
    int value;
    struct ValueNode* next;
} ValueNode;


// 定义用于存储键和值列表的键值对
typedef struct KeyValuePair {
    char* key;
    ValueNode* values;
    struct KeyValuePair* next;
} KeyValuePair;


// 定义 MapSet 结构
typedef struct {
    KeyValuePair* head; // 使用链表存储键值对
    size_t size;
} MapSet;

// 定义 MapMultiMap 结构体
typedef struct {
    KeyValuePair* head;
} MultiMap;


MapSet *mapset_create(size_t initial_capacity);
int mapset_insert(MapSet *mapset, const char *key, int value);
int mapset_find(MapSet *mapset, const char *key, int *value);
int mapset_remove(MapSet *mapset, const char *key);
void mapset_destroy(MapSet *mapset);

/******************************************/
// 创建一个新的 multimap
MultiMap* create_multimap();
// 插入键值对
void multimap_insert(MultiMap* map, const char* key, int value);
// 查找所有与键关联的值
void multimap_find(MultiMap* map, const char* key);
// 删除与键关联的所有值
void multimap_remove(MultiMap* map, const char* key);
// 销毁 multimap
void destroy_multimap(MultiMap* map);
#endif //__MAPSET_H_

