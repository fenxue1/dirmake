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

// 定义队列结构
// CList 结构体：链表的抽象操作接口
typedef struct CList {
    // 函数指针成员（链表操作接口）
    void* (*add)(struct CList *l, void *o);          // 添加元素到链表尾部
    void  (*remove)(struct CList *list, void *obj);         // 移除第n个元素
    void* (*at)(struct CList *l, int n);             // 获取第n个元素的指针
    int   (*realloc)(struct CList *l, int n);        // 调整链表内存容量
    int   (*count)(struct CList *l);                 // 获取链表元素总数    
    void* (*firstMatch)(struct CList *l, const void *o, size_t shift); // 查找第一个匹配元素                  
    void* (*lastMatch)(struct CList *l, const void *o, size_t shift, int n);// 查找最后一个匹配元素
    int   (*index)(struct CList *l, void *o, int n); // 查找元素位置
    int   (*swap)(struct CList *l, int a, int b);    // 交换两个位置的元素
    int   (*allSize)(struct CList *l);               // 获取链表总内存占用
    size_t(*iteSize)(struct CList *l);               // 获取单个元素大小
    void  (*print)(struct CList *l,size_t shift, int n, const char *type); // 格式化打印链表内容
                 
    void  (*clear)(struct CList *l);                // 清空链表元素
    void  (*free)(struct CList *l);                 // 释放整个链表内存
    
    // 关键成员：私有数据指针（隐藏底层实现）
    void* priv;
} CList;

//私有的数据结构 ::内存池
typedef struct ClistMemPool {
    char *pool;           // 内存池起始地址
    size_t pool_size;     // 内存池总大小（字节）
    size_t item_size;     // 单个元素大小
    size_t capacity;      // 容量（元素数量）
    size_t count;         // 当前元素数量
    size_t *free_slots;   // 空闲槽位索引数组
    size_t free_count;    // 空闲槽位数量
}ClistMemPool;




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


// 公共API函数声明

/**
 * 创建新的CList实例
 * @param item_size 单个元素的大小（字节数）
 * @param initial_capacity 初始容量（元素数量），0表示使用默认值
 * @return 新创建的CList指针，失败返回NULL
 */
CList* clist_new(size_t item_size, size_t initial_capacity);



/**
 * 向链表尾部添加元素
 * @param l CList指针
 * @param o 要添加的元素指针
 * @return 添加后元素的指针，失败返回NULL
 */
void* clist_add(CList *list, void *obj);

/**
 * 移除来联表
 * @param lIST CList指针
 * @param obj 要添加的元素指针
 * @return 添加后元素的指针，失败返回NULL
 */
void* clist_remove(CList *list, void *obj);

/**
 * 调整链表内存容量
 * @param list CList指针
 * @param num 新的容量大小
 * @return 成功返回1，失败返回0
 */
int clist_realloc(CList *l, int n);

/**
 * 获取链表元素总数
 * @param l CList指针
 * @return 元素总数
 */
int clist_count(CList *list);

/**
 * 查找第一个匹配的元素
 * @param l CList指针
 * @param o 要匹配的数据指针
 * @param shift 成员偏移量
 * @return 匹配元素的指针，未找到返回NULL
 */
void* clist_firstMatch(CList *list, const void *obj, size_t shift);


/**
 * 查找最后一个匹配的元素
 * @param l CList指针
 * @param o 要匹配的数据指针
 * @param shift 成员偏移量
 * @param n 开始搜索的位置
 * @return 匹配元素的指针，未找到返回NULL
 */
void* clist_lastMatch(CList *list, const void *obj, size_t shift, int n);

/**
 * 查找元素在链表中的位置
 * @param l CList指针
 * @param o 要查找的元素指针
 * @param n 开始搜索的位置
 * @return 元素索引，未找到返回-1
 */
int clist_index(CList *list, void *obj, int n);


/**
 * 交换两个位置的元素
 * @param l CList指针
 * @param a 第一个元素的索引
 * @param b 第二个元素的索引
 * @return 成功返回1，失败返回0
 */
int clist_swap(CList *list, int a, int b);


/**
 * 获取链表总内存占用
 * @param l CList指针
 * @return 总内存占用字节数
 */
int clist_allSize(CList *list);

/**
 * 获取单个元素大小
 * @param l CList指针
 * @return 单个元素大小（字节数）
 */
size_t clist_iteSize(CList *list);

/**
 * 格式化打印链表内容
 * @param l CList指针
 * @param shift 成员偏移量
 * @param n 要打印的元素数量（-1表示全部）
 * @param type 数据类型描述符（"int", "float", "char*"等）
 */
void clist_print(CList *list, size_t shift, int n, const char *type);

/**
 * 清空链表所有元素
 * @param l CList指针
 */
void clist_clear(CList *list);



/**
 * 释放整个链表内存
 * @param l CList指针
 */
void clist_free(CList *list);

#endif // LIST_H


