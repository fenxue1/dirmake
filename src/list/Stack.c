#include "list.h"
#include <stdlib.h>
#include "tr_text.h"

static const _Tr_TEXT txt_input_points_12555= {
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



// 创建一个新的栈
Stack* create_stack() {
    Stack* stack = (Stack*)malloc(sizeof(Stack));
    if (!stack) {
        return NULL;
    }
    stack->list = create_list();
    return stack;
}

// 入栈操作
void stack_push(Stack* stack, void* value) {
    list_append(stack->list, value);  // 使用 list_append 将元素添加到链表尾部
}

// 出栈操作
void* stack_pop(Stack* stack) {
    if (stack->list->size == 0) {
        return NULL;  // 如果栈为空，返回 NULL
    }
    Node* tail_node = stack->list->tail;
    void* value = tail_node->data;
    list_remove(stack->list, tail_node);  // 从链表中移除尾部节点
    return value;
}

// 查看栈顶元素
void* stack_peek(const Stack* stack) {
    if (stack->list->size == 0) {
        return NULL;  // 如果栈为空，返回 NULL
    }
    return stack->list->tail->data;  // 返回尾部节点的数据
}

// 释放栈
void free_stack(Stack* stack) {
    free_list(stack->list);  // 释放链表
    free(stack);  // 释放栈结构体
}

