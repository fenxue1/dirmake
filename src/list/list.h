/*
 * @Author: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @Date: 2024-11-18 00:19:29
 * @LastEditors: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @LastEditTime: 2024-12-05 20:35:48
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


typedef struct {
    List* list; // 使用链表作为底层存储
} Deque;
// 函数声明
List* create_list();
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
// 函数声明：create_stack
// 该函数用于创建一个 Stack 类型的对象
// 返回值：指向 Stack 结构体的指针
Stack* create_stack();
// 函数声明：stack_push
// 该函数用于将元素添加到栈（Stack）中
// 参数：
//    stack：指向栈（Stack）结构体的指针，表示要操作的栈
//    value：指向要添加元素的指针，元素的类型为 void*，可以是任意类型的数据
void stack_push(Stack* stack, void* value);
// 函数声明：stack_pop
// 该函数用于从栈（Stack）中弹出元素
// 参数：
//    stack：指向栈（Stack）结构体的指针，表示要操作的栈
// 返回值：
//    void* 类型的指针，指向从栈中弹出的元素，元素的类型为 void*，可以是任意类型的数据
void* stack_pop(Stack* stack);
// 函数声明：stack_peek
// 该函数用于查看栈（Stack）的栈顶元素，但不将其从栈中移除
// 参数：
//    stack：指向栈（Stack）结构体的指针，表示要操作的栈
// 返回值：
//    void* 类型的指针，指向栈顶元素，元素的类型为 void*，可以是任意类型的数据
void* stack_peek(const Stack* stack);
// 函数声明：free_stack
// 该函数用于释放栈（Stack）所占用的内存
// 参数：
//    stack：指向栈（Stack）结构体的指针，表示要释放内存的栈
void free_stack(Stack* stack);
/*********************************/

// 函数声明：create_deque
// 该函数用于创建一个 Deque 类型的对象
// 返回值：指向 Deque 结构体的指针
Deque* create_deque();
// 函数声明：deque_push_front
// 该函数用于将元素添加到双端队列（Deque）的前端
// 参数：
//    deque：指向双端队列（Deque）结构体的指针，表示要操作的双端队列
//    value：指向要添加元素的指针，元素的类型为 void*，可以是任意类型的数据
void deque_push_front(Deque* deque, void* value);
// 函数声明：deque_push_back
// 该函数用于将元素添加到双端队列（Deque）的后端
// 参数：
//    deque：指向双端队列（Deque）结构体的指针，表示要操作的双端队列
//    value：指向要添加元素的指针，元素的类型为 void*，可以是任意类型的数据
void deque_push_back(Deque* deque, void* value);
// 函数声明：deque_pop_front
// 该函数用于从双端队列（Deque）的前端移除元素
// 参数：
//    deque：指向双端队列（Deque）结构体的指针，表示要操作的双端队列
// 返回值：
//    void* 类型的指针，指向从双端队列前端移除的元素，元素的类型为 void*，可以是任意类型的数据
void* deque_pop_front(Deque* deque);
// 函数声明：deque_pop_back
// 该函数用于从双端队列（Deque）的后端移除元素
void* deque_pop_back(Deque* deque);
// 函数声明：deque_front
// 该函数用于获取双端队列（Deque）的前端元素
void* deque_front(Deque* deque);
// 函数声明：deque_back
// 该函数用于获取双端队列（Deque）的后端元素
void* deque_back(Deque* deque);
// 函数声明：free_deque
// 该函数用于释放双端队列（Deque）所占用的内存
void free_deque(Deque* deque);
/*********************************/
#endif // LIST_H