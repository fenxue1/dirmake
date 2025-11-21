#include "mapset.h"
#include "tr_text.h"

static const _Tr_TEXT txt_input_points_9996633= {
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

// 创建一个新的 MapSet
MapSet *mapset_create(size_t initial_capacity) {
   MapSet* mapset = (MapSet*)malloc(sizeof(MapSet));
    if (!mapset) return NULL;
    mapset->head = NULL;
    mapset->size = 0;
    return mapset;
}

// 插入键值对
int mapset_insert(MapSet *mapset, const char *key, int value) {
      KeyValuePair* current = mapset->head;
    while (current) {
        if (strcmp(current->key, key) == 0) {
            // 键已存在，添加到值链表
            ValueNode* newValue = (ValueNode*)malloc(sizeof(ValueNode));
            if (!newValue) return -1;
            newValue->value = value;
            newValue->next = current->values;
            current->values = newValue;
            return 0;
        }
        current = current->next;
    }

    // 键不存在，创建新键值对
    KeyValuePair* newPair = (KeyValuePair*)malloc(sizeof(KeyValuePair));
    if (!newPair) return -1;
    newPair->key = strdup(key);
    newPair->values = (ValueNode*)malloc(sizeof(ValueNode));
    if (!newPair->values) {
        free(newPair->key);
        free(newPair);
        return -1;
    }
    newPair->values->value = value;
    newPair->values->next = NULL;
    newPair->next = mapset->head;
    mapset->head = newPair;
    mapset->size++;
    return 0;
}

// 查找值
int mapset_find(MapSet *mapset, const char *key, int *value) {
    KeyValuePair* current = mapset->head;
    while (current) {
        if (strcmp(current->key, key) == 0) {
            if (current->values) {
                *value = current->values->value;
                return 0;
            }
            return -1; // 没有值
        }
        current = current->next;
    }
    return -1; // 未找到
}

// 删除键值对
int mapset_remove(MapSet *mapset, const char *key) {
    KeyValuePair* current = mapset->head;
    KeyValuePair* prev = NULL;
    while (current) {
        if (strcmp(current->key, key) == 0) {
            // 找到键，删除
            if (prev) {
                prev->next = current->next;
            } else {
                mapset->head = current->next;
            }
            // 释放值链表
            ValueNode* valueNode = current->values;
            while (valueNode) {
                ValueNode* temp = valueNode;
                valueNode = valueNode->next;
                free(temp);
            }
            free(current->key);
            free(current);
            mapset->size--;
            return 0;
        }
        prev = current;
        current = current->next;
    }
    return -1; // 未找到
}

// 销毁 MapSet
void mapset_destroy(MapSet *mapset) {
   KeyValuePair* current = mapset->head;
    while (current) {
        KeyValuePair* temp = current;
        current = current->next;
        // 释放值链表
        ValueNode* valueNode = temp->values;
        while (valueNode) {
            ValueNode* vtemp = valueNode;
            valueNode = valueNode->next;
            free(vtemp);
        }
        free(temp->key);
        free(temp);
    }
    free(mapset);
}