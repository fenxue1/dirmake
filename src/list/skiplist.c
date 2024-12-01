#include "list.h"

SkipList* create_skiplist() {
    SkipList *list = (SkipList *)malloc(sizeof(SkipList));
    list->level = 0;
    list->header = (Node *)malloc(sizeof(Node));
    list->header->key = -1;  // 头节点的键值设为-1
    list->header->forward = (Node **)malloc(sizeof(Node *) * MAX_LEVEL);
    
    for (int i = 0; i < MAX_LEVEL; i++) {
        list->header->forward[i] = NULL;
    }
    
    return list;
}

//生成随机层数
int SkipList_random_level() {
    int level = 0;
    while ((rand() % 100) < (P * 100) && level < MAX_LEVEL - 1) {
        level++;
    }
    return level;
}



void SkipList_insert(SkipList *list, int key) {
    Node *update[MAX_LEVEL];
    Node *current = list->header;

    // 从最高层开始查找
    for (int i = list->level; i >= 0; i--) {
        while (current->forward[i] != NULL && current->forward[i]->key < key) {
            current = current->forward[i];
        }
        update[i] = current;  // 记录每层的前驱节点
    }

    current = current->forward[0];  // 移动到下一层

    // 如果当前节点为空或键值不等于要插入的键值，则插入新节点
    if (current == NULL || current->key != key) {
        int new_level = SkipList_random_level();

        // 如果新节点的层数大于当前跳表的层数，则更新跳表的层数
        if (new_level > list->level) {
            for (int i = list->level + 1; i <= new_level; i++) {
                update[i] = list->header;  // 更新前驱节点为头节点
            }
            list->level = new_level;
        }

        // 创建新节点
        Node *new_node = (Node *)malloc(sizeof(Node));
        new_node->key = key;
        new_node->forward = (Node **)malloc(sizeof(Node *) * (new_level + 1));

        // 初始化新节点的指针
        for (int i = 0; i <= new_level; i++) {
            new_node->forward[i] = NULL;
        }

        // 更新前驱节点的指针
        for (int i = 0; i <= new_level; i++) {
            new_node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = new_node;
        }
    }
}


//查找元素
Node* SkipList_search(SkipList *list, int key) {
    Node *current = list->header;

    // 从最高层开始查找
    for (int i = list->level; i >= 0; i--) {
        while (current->forward[i] != NULL && current->forward[i]->key < key) {
            current = current->forward[i];
        }
    }

    current = current->forward[0];  // 移动到下一层

    // 检查当前节点是否为目标节点
    if (current != NULL && current->key == key) {
        return current;  // 找到节点
    }
    return NULL;  // 未找到
}



void SkipList_delete(SkipList *list, int key) {
    Node *update[MAX_LEVEL];
    Node *current = list->header;

    // 从最高层开始查找
    for (int i = list->level; i >= 0; i--) {
        while (current->forward[i] != NULL && current->forward[i]->key < key) {
            current = current->forward[i];
        }
        update[i] = current;  // 记录每层的前驱节点
    }

    current = current->forward[0];  // 移动到下一层

    // 如果当前节点为目标节点，则删除
    if (current != NULL && current->key == key) {
        for (int i = 0; i <= list->level; i++) {
            if (update[i]->forward[i] != current) {
                break;
            }
            update[i]->forward[i] = current->forward[i];  // 更新前驱节点的指针
        }

        // 处理层数减少的情况
        while (list->level > 0 && list->header->forward[list->level] == NULL) {
            list->level--;
        }

        free(current->forward);
        free(current);
    }
}


void destroy_skiplist(SkipList *list) {
    Node *current = list->header;
    Node *next;

    // 遍历并释放所有节点
    while (current != NULL) {
        next = current->forward[0];
        free(current->forward);
        free(current);
        current = next;
    }
    free(list);
}