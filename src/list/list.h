/*
 * @Author: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @Date: 2024-11-18 00:19:29
 * @LastEditors: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @LastEditTime: 2024-11-27 23:05:53
 * @FilePath: \test_cmake\src\list\list.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef LIST_H
#define LIST_H

#include <stddef.h> // 包含 size_t 的定义、
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../mempool/mempool.h"


#define MAX_LEVEL 16
#define P 0.5  // 0.5的概率决定了每个元素的层数
// 定义双向链表节点结构
typedef struct Node {
    void* data;              // 节点数据，使用 void* 指针
    struct Node* next;      // 指向下一个节点的指针
    struct Node* prev;      // 指向前一个节点的指针
    int key;
    struct Node **forward;  // 指向下一层的指针数组
} Node;

// 定义双向链表结构
typedef struct {
    Node* head;             // 链表头指针
    Node* tail;             // 链表尾指针
    size_t size;            // 链表大小
    MemoryPool* node_pool;  // 节点内存池
} List;

typedef struct SkipList {
    int level;             // 当前跳表的最大层数
    Node *header;         // 跳表的头节点
} SkipList;


typedef struct {
    List* list;  // 使用 List 作为栈的底层数据结构
} Stack;

// 函数声明
List* create_list();
// 函数：create_mempool_list
// 功能：创建一个使用内存池的链表
// 参数：
// - max_nodes：表示内存池允许的最大节点数量
// 注意：此函数可能会根据传入的最大节点数量创建一个内存池，并将其与链表关联，
// 以提高内存分配和释放的效率，避免频繁的系统调用。
// 可能会涉及到内存池的初始化操作，以及链表结构的初始化，
// 例如设置链表的头指针、尾指针、大小等信息。
List *create_mempool_list(size_t max_nodes);
void list_append(List* list, void* value);
void list_prepend(List* list, void* value);
void list_delete_node(List* list, Node* node);
void* list_remove(List *list, Node *node);
void list_print_list(List* list, void (*print_func)(void*));
int list_remove_at(List* list, size_t position);
// 返回第一个节点
Node* list_node_first(List* list); 
 // 返回最后一个节点
Node* list_node_last(List* list); 
// 分割链表为两个部分
void list_split(Node* source, Node** front, Node** back);
// 合并两个已排序的链表
Node* list_node_merge(Node* left, Node* right);
// 归并排序
void Node_merge_sort(Node** head);
// 排序链表
void list_sort(List* list);

// 函数：list_insert_sorted
// 功能：将元素按照一定的顺序插入到链表中
// 参数：
// - list：指向 List 结构体的指针，表示要操作的链表
// - value：要插入的元素，使用 void* 指针表示，可以存储任意类型的数据
// 注意：此函数的具体实现可能需要根据元素的比较规则来确定插入位置，以保证链表的有序性
// 例如，如果存储的是整数，可以根据整数的大小进行排序；如果存储的是字符串，可以根据字符串的字典序进行排序
// 该函数的实现可能会涉及遍历链表，找到合适的插入位置，然后更新节点的前后指针
void list_insert_sorted(List *list, void* value);

// 反向打印链表
void list_print_reverse_list(List* list, void (*print_func)(void*));

void free_list(List* list);

/*********************************/
//一下是跳表
SkipList* create_skiplist();
int SkipList_random_level();
void SkipList_insert(SkipList *list, int key);
Node* SkipList_search(SkipList *list, int key);
void SkipList_delete(SkipList *list, int key);
void destroy_skiplist(SkipList *list);

/*********************************/
// 函数声明
Stack* create_stack();
void stack_push(Stack* stack, void* value);
void* stack_pop(Stack* stack);
void* stack_peek(const Stack* stack);
void free_stack(Stack* stack);
/*********************************/
#endif // LIST_H