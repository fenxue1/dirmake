#include <stdio.h>
#include <stdlib.h>
#include "list.h"
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
void list_append(List *list, void* value)
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
void list_prepend(List *list, void* value)
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
void* list_remove(List *list, Node *node)
{
   
if (node == NULL)
        return NULL; // 返回 NULL 表示错误

    void* value = node->data; // 保存要删除的节点的数据

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

    void* value = current->data; // 保存要删除的节点的数据

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
void list_print_list(List* list, void (*print_func)(void*)) {
    Node* current = list->head;
    while (current != NULL) {
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
void list_insert_sorted(List *list, void* value)
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
void list_print_reverse_list(List* list, void (*print_func)(void*)) {
    Node* current = list->tail;
    while (current != NULL) {
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
