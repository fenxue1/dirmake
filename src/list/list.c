#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "tr_text.h"

static const _Tr_TEXT txt_input_points_999999 = {
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

// 内存池配置
#define DEFAULT_INITIAL_CAPACITY 8
#define MIN_CAPACITY 4
#define GROWTH_FACTOR 2

// 内部实现函数声明
static void *_clist_add(CList *l, void *o);
static void _clist_remove(CList *l, int n);
static void *_clist_at(CList *l, int n);
static int _clist_realloc(CList *l, int n);
static int _clist_count(CList *l);
static void *_clist_firstMatch(CList *l, const void *o, size_t shift);
static void *_clist_lastMatch(CList *l, const void *o, size_t shift, int n);
static int _clist_index(CList *l, void *o, int n);
static int _clist_swap(CList *l, int a, int b);
static int _clist_allSize(CList *l);
static size_t _clist_iteSize(CList *l);
static void _clist_print(CList *l, size_t shift, int n, const char *type);
static void _clist_clear(CList *l);
static void _clist_free(CList *l);

// 内存池辅助函数
static int _pool_expand(ClistMemPool *pool, size_t new_capacity);
static void *_pool_get_slot(ClistMemPool *pool, size_t index);
static int _pool_is_slot_used(ClistMemPool *pool, size_t index);
static void _pool_mark_slot_used(ClistMemPool *pool, size_t index);
static void _pool_mark_slot_free(ClistMemPool *pool, size_t index);

// 创建一个新的双向链表
List *create_list()
{
    List *list = (List *)malloc(sizeof(List));
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    list->node_pool = mempool_create(sizeof(Node), 10000); // 创建内存池，假设最多100个节点
    if (!list->node_pool)
    {
        free(list);
        return NULL;
    }
    return list;
}

// 在链表末尾添加一个新节点
void list_append(List *list, void *value)
{
    Node *new_node = (Node *)mempool_alloc(list->node_pool);
    if (!new_node)
        return; // 内存池已满

    new_node->data = value;
    new_node->next = NULL;

    if (list->tail == NULL)
    { // 链表为空
        new_node->prev = NULL;
        list->head = new_node;
        list->tail = new_node;
    }
    else
    {
        new_node->prev = list->tail;
        list->tail->next = new_node;
        list->tail = new_node;
    }
    list->size++;
}

// 在链表开头添加一个新节点
void list_prepend(List *list, void *value)
{
    // Node* new_node = (Node*)malloc(sizeof(Node));
    Node *new_node = (Node *)mempool_alloc(list->node_pool);
    if (!new_node)
        return; // 内存池已满
    new_node->data = value;
    new_node->prev = NULL;

    if (list->head == NULL)
    { // 链表为空
        new_node->next = NULL;
        list->head = new_node;
        list->tail = new_node;
    }
    else
    {
        new_node->next = list->head;
        list->head->prev = new_node;
        list->head = new_node;
    }
    list->size++;
}

// 删除指定节点
void list_delete_node(List *list, Node *node)
{
    if (node == NULL)
        return;

    if (node->prev != NULL)
    {
        node->prev->next = node->next;
    }
    else
    {
        list->head = node->next; // 删除头节点
    }

    if (node->next != NULL)
    {
        node->next->prev = node->prev;
    }
    else
    {
        list->tail = node->prev; // 删除尾节点
    }

    // free(node);
    mempool_free(list->node_pool, node); // 释放节点回内存池
    list->size--;
}

// 删除指定节点并返回其值
void *list_remove(List *list, Node *node)
{

    if (node == NULL)
        return NULL; // 返回 NULL 表示错误

    void *value = node->data; // 保存要删除的节点的数据

    if (node->prev != NULL)
    {
        node->prev->next = node->next;
    }
    else
    {
        list->head = node->next; // 删除头节点
    }

    if (node->next != NULL)
    {
        node->next->prev = node->prev;
    }
    else
    {
        list->tail = node->prev; // 删除尾节点
    }

    mempool_free(list->node_pool, node); // 释放节点回内存池
    list->size--;

    return value; // 返回被删除节点的值
}

// 根据位置删除节点
int list_remove_at(List *list, size_t position)
{
    if (position >= list->size)
    {
        fprintf(stderr, "Error: Position out of bounds.\n");
        return -1; // 返回错误值
    }

    Node *current = list->head;
    for (size_t i = 0; i < position; i++)
    {
        current = current->next; // 找到指定位置的节点
    }

    void *value = current->data; // 保存要删除的节点的数据

    if (current->prev != NULL)
    {
        current->prev->next = current->next;
    }
    else
    {
        list->head = current->next; // 删除头节点
    }

    if (current->next != NULL)
    {
        current->next->prev = current->prev;
    }
    else
    {
        list->tail = current->prev; // 删除尾节点
    }

    mempool_free(list->node_pool, current); // 释放节点回内存池
    list->size--;

    return (int)(size_t)value; // 返回被删除节点的值（假设值是整数）
}

// 打印链表
void list_print_list(List *list, void (*print_func)(void *))
{
    Node *current = list->head;
    while (current != NULL)
    {
        print_func(current->data); // 使用传入的打印函数
        current = current->next;
    }
    printf("\n");
}
// 返回链表的第一个节点
Node *list_node_first(List *list)
{
    return list->head; // 返回头指针
}

// 返回链表的最后一个节点
Node *list_node_last(List *list)
{
    return list->tail; // 返回尾指针
}

// 分割链表为两个部分
void list_split(Node *source, Node **front, Node **back)
{
    Node *fast;
    Node *slow;
    slow = source;
    fast = source->next;

    // 快慢指针法
    while (fast != NULL)
    {
        fast = fast->next;
        if (fast != NULL)
        {
            slow = slow->next;
            fast = fast->next;
        }
    }

    *front = source;
    *back = slow->next;
    slow->next = NULL;
}

// 合并两个已排序的链表
Node *list_node_merge(Node *left, Node *right)
{
    if (!left)
        return right;
    if (!right)
        return left;

    if (left->data < right->data)
    {
        left->next = list_node_merge(left->next, right);
        left->next->prev = left;
        left->prev = NULL;
        return left;
    }
    else
    {
        right->next = list_node_merge(left, right->next);
        right->next->prev = right;
        right->prev = NULL;
        return right;
    }
}

// 归并排序
void Node_merge_sort(Node **head)
{
    Node *current = *head;
    Node *left;
    Node *right;

    if (current == NULL || current->next == NULL)
    {
        return;
    }

    list_split(current, &left, &right);

    Node_merge_sort(&left);
    Node_merge_sort(&right);

    *head = list_node_merge(left, right);
}

// 排序链表
void list_sort(List *list)
{
    Node_merge_sort(&list->head);

    // 更新尾指针
    Node *current = list->head;
    while (current && current->next != NULL)
    {
        current = current->next;
    }
    list->tail = current;
}

// 有序插入函数，假设存储的是整数
void list_insert_sorted(List *list, void *value)
{
    Node *new_node = (Node *)mempool_alloc(list->node_pool);
    if (!new_node)
        return; // 内存池已满

    new_node->data = value;
    new_node->next = NULL;
    new_node->prev = NULL;

    // 如果链表为空
    if (list->head == NULL)
    {
        list->head = new_node;
        list->tail = new_node;
    }
    else
    {
        Node *current = list->head;
        // 遍历找到插入位置
        while (current != NULL && (int)(size_t)current->data < (int)(size_t)value)
        {
            current = current->next;
        }

        if (current == NULL)
        {
            // 插入到尾部
            new_node->prev = list->tail;
            list->tail->next = new_node;
            list->tail = new_node;
        }
        else if (current == list->head)
        {
            // 插入到头部
            new_node->next = list->head;
            list->head->prev = new_node;
            list->head = new_node;
        }
        else
        {
            // 插入到中间
            new_node->next = current;
            new_node->prev = current->prev;
            current->prev->next = new_node;
            current->prev = new_node;
        }
    }
    list->size++;
}

// 反向打印链表
void list_print_reverse_list(List *list, void (*print_func)(void *))
{
    Node *current = list->tail;
    while (current != NULL)
    {
        print_func(current->data); // 使用传入的打印函数
        current = current->prev;
    }
    printf("\n");
}

// 释放链表的内存
void free_list(List *list)
{
    Node *current = list->head;
    Node *next_node;

    while (current != NULL)
    {
        next_node = current->next;
        mempool_free(list->node_pool, current); // 释放节点回内存池
        current = next_node;
    }

    mempool_destroy(list->node_pool); // 销毁内存池
    free(list);
}

// ClistMemPool

// ================= 公共API函数实现 =================

CList *clist_new(size_t item_size, size_t initial_capacity)
{
    if (item_size == 0)
        return NULL;

    // 分配CList结构
    CList *list = malloc(sizeof(CList));
    if (!list)
        return NULL;

    // 分配内存池结构
    ClistMemPool *pool = malloc(sizeof(ClistMemPool));
    if (!pool)
    {
        free(list);
        return NULL;
    }

    // 确定初始容量
    size_t capacity = initial_capacity > 0 ? initial_capacity : DEFAULT_INITIAL_CAPACITY;
    if (capacity < MIN_CAPACITY)
        capacity = MIN_CAPACITY;

    // 初始化内存池
    pool->item_size = item_size;
    pool->capacity = capacity;
    pool->count = 0;
    pool->pool_size = capacity * item_size;

    // 分配内存池和空闲槽位数组
    pool->pool = malloc(pool->pool_size);
    pool->free_slots = malloc(capacity * sizeof(size_t));

    if (!pool->pool || !pool->free_slots)
    {
        free(pool->pool);
        free(pool->free_slots);
        free(pool);
        free(list);
        return NULL;
    }

    // 初始化所有槽位为空闲
    pool->free_count = capacity;
    for (size_t i = 0; i < capacity; i++)
    {
        pool->free_slots[i] = i;
    }

    // 清零内存池
    memset(pool->pool, 0, pool->pool_size);

    // 设置函数指针
    list->add = _clist_add;
    list->remove = _clist_remove;
    list->at = _clist_at;
    list->realloc = _clist_realloc;
    list->count = _clist_count;
    list->firstMatch = _clist_firstMatch;
    list->lastMatch = _clist_lastMatch;
    list->index = _clist_index;
    list->swap = _clist_swap;
    list->allSize = _clist_allSize;
    list->iteSize = _clist_iteSize;
    list->print = _clist_print;
    list->clear = _clist_clear;
    list->free = _clist_free;
    list->priv = pool;

    return list;
}

// ================= 内存池辅助函数 =================

// 扩展内存池容量
static int _pool_expand(ClistMemPool *pool, size_t new_capacity)
{
    if (new_capacity <= pool->capacity)
        return 1;

    // 重新分配内存池
    char *new_pool = realloc(pool->pool, new_capacity * pool->item_size);
    if (!new_pool)
        return 0;

    // 重新分配空闲槽位数组
    size_t *new_free_slots = realloc(pool->free_slots, new_capacity * sizeof(size_t));
    if (!new_free_slots)
    {
        // 回滚内存池分配
        char *old_pool = realloc(new_pool, pool->capacity * pool->item_size);
        if (old_pool)
            pool->pool = old_pool;
        return 0;
    }

    // 更新内存池信息
    pool->pool = new_pool;
    pool->free_slots = new_free_slots;

    // 清零新分配的内存
    memset(pool->pool + pool->capacity * pool->item_size, 0,
           (new_capacity - pool->capacity) * pool->item_size);

    // 添加新的空闲槽位
    for (size_t i = pool->capacity; i < new_capacity; i++)
    {
        pool->free_slots[pool->free_count++] = i;
    }

    pool->capacity = new_capacity;
    pool->pool_size = new_capacity * pool->item_size;

    return 1;
}

// 获取指定槽位的内存地址
static void *_pool_get_slot(ClistMemPool *pool, size_t index)
{
    if (index >= pool->capacity)
        return NULL;
    return pool->pool + index * pool->item_size;
}

// 检查槽位是否被使用
static int _pool_is_slot_used(ClistMemPool *pool, size_t index)
{
    if (index >= pool->capacity)
        return 0;

    // 检查是否在空闲列表中
    for (size_t i = 0; i < pool->free_count; i++)
    {
        if (pool->free_slots[i] == index)
        {
            return 0; // 在空闲列表中，说明未使用
        }
    }
    return 1; // 不在空闲列表中，说明正在使用
}

// 标记槽位为已使用
static void _pool_mark_slot_used(ClistMemPool *pool, size_t index)
{
    // 从空闲槽位数组中移除
    for (size_t i = 0; i < pool->free_count; i++)
    {
        if (pool->free_slots[i] == index)
        {
            // 将最后一个空闲槽位移动到当前位置
            pool->free_slots[i] = pool->free_slots[--pool->free_count];
            break;
        }
    }
}

// 标记槽位为空闲
static void _pool_mark_slot_free(ClistMemPool *pool, size_t index)
{
    // 检查是否已经在空闲列表中
    for (size_t i = 0; i < pool->free_count; i++)
    {
        if (pool->free_slots[i] == index)
        {
            return; // 已经是空闲状态
        }
    }

    // 添加到空闲槽位数组
    if (pool->free_count < pool->capacity)
    {
        pool->free_slots[pool->free_count++] = index;
    }

    // 清零槽位内容
    void *slot = _pool_get_slot(pool, index);
    if (slot)
    {
        memset(slot, 0, pool->item_size);
    }
}

// ================= 内部实现函数 =================

// 添加元素到链表尾部
static void *_clist_add(CList *l, void *o)
{
    if (!l || !l->priv || !o)
        return NULL;

    ClistMemPool *pool = (ClistMemPool *)l->priv;

    // 检查是否需要扩容
    if (pool->free_count == 0)
    {
        size_t new_capacity = pool->capacity * GROWTH_FACTOR;
        if (!_pool_expand(pool, new_capacity))
        {
            return NULL;
        }
    }

    // 获取一个空闲槽位
    size_t slot_index = pool->free_slots[0];
    void *slot = _pool_get_slot(pool, slot_index);
    if (!slot)
        return NULL;

    // 复制数据到槽位
    memcpy(slot, o, pool->item_size);

    // 标记槽位为已使用
    _pool_mark_slot_used(pool, slot_index);
    pool->count++;

    return slot;
}

// 移除第n个元素
static void _clist_remove(CList *l, int n)
{
    if (!l || !l->priv || n < 0)
        return;

    ClistMemPool *pool = (ClistMemPool *)l->priv;
    if (n >= (int)pool->count)
        return;

    // 找到第n个被使用的槽位
    size_t used_count = 0;
    for (size_t i = 0; i < pool->capacity; i++)
    {
        if (_pool_is_slot_used(pool, i))
        {
            if (used_count == (size_t)n)
            {
                // 找到了要删除的槽位
                _pool_mark_slot_free(pool, i);
                pool->count--;
                return;
            }
            used_count++;
        }
    }
}

// 获取第n个元素的指针
static void *_clist_at(CList *l, int n)
{
    if (!l || !l->priv || n < 0)
    {
        printf("Invalid list or negative index\n");
        return NULL;
    }

    ClistMemPool *pool = (ClistMemPool *)l->priv;
    if (n >= (int)pool->count)
    {
        printf("Index %d out of bounds (count=%zu)\n", n, pool->count);
        return NULL;
    }

    for (size_t i = 0, used = 0; i < pool->capacity; i++)
    {
        if (_pool_is_slot_used(pool, i))
        {
            if (used++ == (size_t)n)
            {
                void *slot = _pool_get_slot(pool, i);
                if (!slot)
                    printf("Slot %zu is NULL\n", i);
                return slot;
            }
        }
    }

    printf("No slot found for index %d\n", n);
    return NULL;
}

// 调整链表内存容量
static int _clist_realloc(CList *l, int n)
{
    if (!l || !l->priv || n < 0)
        return 0;

    ClistMemPool *pool = (ClistMemPool *)l->priv;

    if (n == 0)
    {
        // 释放所有内存但保留结构
        _clist_clear(l);
        return 1;
    }

    size_t new_capacity = (size_t)n;
    if (new_capacity < pool->count)
    {
        // 新容量小于当前元素数量，需要删除多余元素
        while (pool->count > new_capacity)
        {
            _clist_remove(l, (int)pool->count - 1);
        }
    }

    return _pool_expand(pool, new_capacity);
}

// 获取链表元素总数
static int _clist_count(CList *l)
{
    if (!l || !l->priv)
        return 0;

    ClistMemPool *pool = (ClistMemPool *)l->priv;
    return (int)pool->count;
}

// 查找第一个匹配元素
static void *_clist_firstMatch(CList *l, const void *o, size_t shift)
{
    if (!l || !l->priv || !o)
        return NULL;

    ClistMemPool *pool = (ClistMemPool *)l->priv;

    for (size_t i = 0; i < pool->capacity; i++)
    {
        if (_pool_is_slot_used(pool, i))
        {
            void *item = _pool_get_slot(pool, i);
            if (item && memcmp((char *)item + shift, o, pool->item_size - shift) == 0)
            {
                return item;
            }
        }
    }

    return NULL;
}

// 查找最后一个匹配元素
static void *_clist_lastMatch(CList *l, const void *o, size_t shift, int n)
{
    if (!l || !l->priv || !o)
        return NULL;

    ClistMemPool *pool = (ClistMemPool *)l->priv;
    void *last_match = NULL;
    size_t used_count = 0;
    int search_limit = (n >= 0 && n < (int)pool->count) ? n + 1 : (int)pool->count;

    for (size_t i = 0; i < pool->capacity && used_count < (size_t)search_limit; i++)
    {
        if (_pool_is_slot_used(pool, i))
        {
            void *item = _pool_get_slot(pool, i);
            if (item && memcmp((char *)item + shift, o, pool->item_size - shift) == 0)
            {
                last_match = item;
            }
            used_count++;
        }
    }

    return last_match;
}

// 查找元素位置
static int _clist_index(CList *l, void *o, int n)
{
    if (!l || !l->priv || !o)
        return -1;

    ClistMemPool *pool = (ClistMemPool *)l->priv;
    size_t used_count = 0;
    int start = (n >= 0 && n < (int)pool->count) ? n : 0;

    for (size_t i = 0; i < pool->capacity; i++)
    {
        if (_pool_is_slot_used(pool, i))
        {
            if ((int)used_count >= start)
            {
                void *item = _pool_get_slot(pool, i);
                if (item == o)
                {
                    return (int)used_count;
                }
            }
            used_count++;
        }
    }

    return -1;
}

// 交换两个位置的元素
static int _clist_swap(CList *l, int a, int b)
{
    if (!l || !l->priv)
        return 0;

    ClistMemPool *pool = (ClistMemPool *)l->priv;
    if (a < 0 || a >= (int)pool->count || b < 0 || b >= (int)pool->count || a == b)
    {
        return 0;
    }

    void *item_a = _clist_at(l, a);
    void *item_b = _clist_at(l, b);

    if (!item_a || !item_b)
        return 0;

    // 创建临时缓冲区交换数据
    void *temp = malloc(pool->item_size);
    if (!temp)
        return 0;

    memcpy(temp, item_a, pool->item_size);
    memcpy(item_a, item_b, pool->item_size);
    memcpy(item_b, temp, pool->item_size);

    free(temp);
    return 1;
}

// 获取链表总内存占用
static int _clist_allSize(CList *l)
{
    if (!l || !l->priv)
        return 0;

    ClistMemPool *pool = (ClistMemPool *)l->priv;
    return (int)(pool->pool_size +
                 pool->capacity * sizeof(size_t) +
                 sizeof(ClistMemPool) +
                 sizeof(CList));
}

// 获取单个元素大小
static size_t _clist_iteSize(CList *l)
{
    if (!l || !l->priv)
        return 0;

    ClistMemPool *pool = (ClistMemPool *)l->priv;
    return pool->item_size;
}

// 格式化打印链表内容
static void _clist_print(CList *l, size_t shift, int n, const char *type)
{
    if (!l || !l->priv)
        return;

    ClistMemPool *pool = (ClistMemPool *)l->priv;
    int count = (n > 0 && n < (int)pool->count) ? n : (int)pool->count;

    printf("CList [count=%zu, capacity=%zu, item_size=%zu, pool_size=%zu, free_slots=%zu]:\n",
           pool->count, pool->capacity, pool->item_size, pool->pool_size, pool->free_count);

    size_t printed = 0;
    for (size_t i = 0; i < pool->capacity && printed < (size_t)count; i++)
    {
        if (_pool_is_slot_used(pool, i))
        {
            void *item = _pool_get_slot(pool, i);
            printf("[%zu@slot%zu] ", printed, i);

            if (type && strcmp(type, "int") == 0)
            {
                printf("%d\n", *(int *)((char *)item + shift));
            }
            else if (type && strcmp(type, "float") == 0)
            {
                printf("%.2f\n", *(float *)((char *)item + shift));
            }
            else if (type && strcmp(type, "char*") == 0)
            {
                printf("%s\n", *(char **)((char *)item + shift));
            }
            else if (type && strcmp(type, "double") == 0)
            {
                printf("%.6f\n", *(double *)((char *)item + shift));
            }
            else if (type && strcmp(type, "char") == 0)
            {
                printf("%c\n", *(char *)((char *)item + shift));
            }
            else
            {
                printf("ptr=%p\n", (char *)item + shift);
            }
            printed++;
        }
    }
}

// 清空链表元素
static void _clist_clear(CList *l)
{
    if (!l || !l->priv)
        return;

    ClistMemPool *pool = (ClistMemPool *)l->priv;

    // 重置所有槽位为空闲
    pool->free_count = pool->capacity;
    for (size_t i = 0; i < pool->capacity; i++)
    {
        pool->free_slots[i] = i;
    }

    // 清零内存池
    memset(pool->pool, 0, pool->pool_size);
    pool->count = 0;
}

// 释放整个链表内存
static void _clist_free(CList *l)
{
    if (!l)
        return;

    if (l->priv)
    {
        ClistMemPool *pool = (ClistMemPool *)l->priv;

        // 释放内存池和相关数组
        free(pool->pool);
        free(pool->free_slots);
        free(pool);
    }

    // 释放CList结构本身
    free(l);
}

// ================= 便捷包装函数 =================

void *clist_add(CList *l, void *o)
{
    return l ? l->add(l, o) : NULL;
}

void *clist_remove(CList *list, void *obj)
{
    if (list)
        list->remove(list, obj);
}

void *clist_at(CList *l, int n)
{
    return l ? l->at(l, n) : NULL;
}

int clist_count(CList *l)
{
    return l ? l->count(l) : 0;
}

void *clist_firstMatch(CList *l, const void *o, size_t shift)
{
    return l ? l->firstMatch(l, o, shift) : NULL;
}

void *clist_lastMatch(CList *l, const void *o, size_t shift, int n)
{
    return l ? l->lastMatch(l, o, shift, n) : NULL;
}

int clist_index(CList *l, void *o, int n)
{
    return l ? l->index(l, o, n) : -1;
}

int clist_swap(CList *l, int a, int b)
{
    return l ? l->swap(l, a, b) : 0;
}

int clist_allSize(CList *l)
{
    return l ? l->allSize(l) : 0;
}

size_t clist_iteSize(CList *l)
{
    return l ? l->iteSize(l) : 0;
}

void clist_print(CList *l, size_t shift, int n, const char *type)
{
    if (l)
        l->print(l, shift, n, type);
}

void clist_clear(CList *l)
{
    if (l)
        l->clear(l);
}

void clist_free(CList *l)
{
    if (l)
        l->free(l);
}

int clist_realloc(CList *l, int n)
{
    return l ? l->realloc(l, n) : 0;
}


