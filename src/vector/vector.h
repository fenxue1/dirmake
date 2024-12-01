/*
 * @Author: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @Date: 2024-11-17 23:31:58
 * @LastEditors: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @LastEditTime: 2024-11-17 23:47:37
 * @FilePath: \test_cmake\src\vector\vector.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef VECTOR_H
#define VECTOR_H

typedef struct {
    int *data;
    size_t size;
    size_t capacity;
} Vector;

Vector* create_vector(size_t capacity);
void vector_push_back(Vector *vector, int value);
int vector_pop(Vector *vector);
void free_vector(Vector *vector);

#endif // VECTOR_H