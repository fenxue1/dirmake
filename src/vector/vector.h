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
    void **data;// 使用 void* 指针数组存储任意类型的数据
    size_t size; // 当前元素数量
    size_t capacity;
} Vector;

Vector* create_vector(size_t capacity);
void vector_push_back(Vector *vector,void *value);
void* vector_pop(Vector *vector);
void free_vector(Vector *vector);

#endif // VECTOR_H



// # 指定源文件
// set(SOURCES
//     src/main.c
//     src/math/add.c
//     src/math/subtract.c
//     src/math/multiply.c  # 添加新的源文件
//     src/utils/utils.c
//     src/vector/vector.c  # 添加新的源文件
//     src/list/list.c  # 添加新的源文件
//     src/list/skiplist.c  # 添加新的源文件
//     src/list/Stack.c  # 添加新的源文件
//     src/list/deque.c  # 添加新的源文件
//     src/test_leetecode/test_leetecode.c
//     src/threadpool/threadpool.c
//     src/mempool/mempool.c
//     src/cjson/cJSON.c
//     src/mapset/mapset.c
//     src/mapset/hashmap.c
//     src/mapset/multimap.c
//     src/ValueRange/ValueRange.c
//     src/common.c  # 添加新的源文件
// )