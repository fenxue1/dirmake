/*
 * @Author: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @Date: 2024-12-05 20:31:00
 * @LastEditors: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @LastEditTime: 2024-12-05 21:26:24
 * @FilePath: \test_cmake\src\list\deque.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "list.h"
#include "tr_text.h"

static const _Tr_TEXT txt_input_points = {
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

//2. 创建双向队列
// 我们需要一个函数来创建一个新的双向队列。
Deque* create_deque() {
    Deque* deque = (Deque*)malloc(sizeof(Deque));
    deque->list = create_list(); // 创建一个新的链表
    return deque;
}
//在队列前端插入元素
void deque_push_front(Deque* deque, void* value) {
    list_prepend(deque->list, value); // 在链表前端插入元素
}


//在队列后端插入元素
void deque_push_back(Deque* deque, void* value) {
    list_append(deque->list, value); // 在链表后端插入元素
}

//从队列前端弹出元素
void* deque_pop_front(Deque* deque) {
    if (deque->list->size == 0) {
        return NULL; // 队列为空
    }
    Node* frontNode = list_node_first(deque->list);
    void* value = frontNode->data; // 保存前端元素

    //list_remove_node(deque->list, frontNode); // 从链表中删除前端元素
    list_delete_node(deque->list,frontNode);
    return value; // 返回被删除的元素
}


void* deque_pop_back(Deque* deque) {
    if (deque->list->size == 0) {
        return NULL; // 队列为空
    }
    Node* backNode = list_node_last(deque->list);
    void* value = backNode->data; // 保存后端元素

    list_delete_node(deque->list, backNode); // 从链表中删除后端元素
    return value; // 返回被删除的元素
}


void* deque_front(Deque* deque) {
    if (deque->list->size == 0) {
        return NULL; // 队列为空
    }
    return list_node_first(deque->list)->data; // 返回前端元素
}


void* deque_back(Deque* deque) {
    if (deque->list->size == 0) {
        return NULL; // 队列为空
    }
    return list_node_last(deque->list)->data; // 返回后端元素
}


void free_deque(Deque* deque) {
    free_list(deque->list); // 释放链表
    free(deque); // 释放队列结构
}
