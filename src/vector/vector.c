/*
 * @Author: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @Date: 2024-11-17 23:29:58
 * @LastEditors: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @LastEditTime: 2024-11-30 01:31:44
 * @FilePath: \test_cmake\src\vector\vector.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include <stdlib.h>
#include "vector.h"
#include <stdio.h> // 包含 printf 的定义
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
Vector* create_vector(size_t capacity) {
    Vector *vector = (Vector *)malloc(sizeof(Vector));
    if (!vector) {
        fprintf(stderr, "Error: Memory allocation failed for Vector.\n");
        return NULL;
    }
    vector->data = (void **)malloc(capacity * sizeof(void *));
    if (!vector->data) {
        fprintf(stderr, "Error: Memory allocation failed for Vector data.\n");
        free(vector);
        return NULL;
    }
    vector->size = 0;
    vector->capacity = capacity;
    return vector;
}

// 向 Vector 添加元素
void vector_push_back(Vector *vector, void *value) {
    if (vector->size >= vector->capacity) {
        // 扩展容量
        size_t new_capacity = vector->capacity * 2;
        void **new_data = (void **)realloc(vector->data, new_capacity * sizeof(void *));
        if (!new_data) {
            fprintf(stderr, "Error: Memory allocation failed during resizing.\n");
            return;
        }
        vector->data = new_data;
        vector->capacity = new_capacity;
    }
    vector->data[vector->size++] = value;
}
/// @brief 
/// @param vector 
/// @return 
// 从 Vector 中弹出元素
void* vector_pop(Vector *vector) {
    if (vector->size == 0) {
        fprintf(stderr, "Error: Attempt to pop from an empty vector.\n");
        return NULL; // 返回 NULL 表示向量为空
    }
    return vector->data[--vector->size]; // 返回最后一个元素并减少大小
}

void free_vector(Vector *vector) {
    free(vector->data);
    free(vector);
}

