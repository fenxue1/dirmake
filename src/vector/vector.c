#include <stdlib.h>
#include "vector.h"
#include <stdio.h> // 包含 printf 的定义
Vector* create_vector(size_t capacity) {
    Vector *vector = (Vector *)malloc(sizeof(Vector));
    vector->data = (int *)malloc(capacity * sizeof(int));
    vector->size = 0;
    vector->capacity = capacity;
    return vector;
}

void vector_push_back(Vector *vector, int value) {
   if (vector->size < vector->capacity) {
        vector->data[vector->size++] = value;
    } else {
        fprintf(stderr, "Error: Attempt to push back to a full vector.\n");
    }
}
int vector_pop(Vector *vector) {
    if (vector->size == 0) {
        fprintf(stderr, "Error: Attempt to pop from an empty vector.\n");
        return -1; // 返回一个错误值，表示向量为空
    }
    return vector->data[--vector->size]; // 返回最后一个元素并减少大小
}


void free_vector(Vector *vector) {
    free(vector->data);
    free(vector);
}