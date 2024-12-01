#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <locale.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>

#include "../src/cjson/cJSON.h" // 确保你有 cJSON 库的头文件
#include "../src/list/list.h"  // 添加新的头文件
#include "../src/mempool/mempool.h"  // 添加新的头文件
#include "../src/test_leetecode/test_leetecode.h"
#include "../src/mapset/mapset.h"
#include "../src/mapset/hashmap.h"

// 定义 Person 结构体
typedef struct {
    char name[50];
    int age;
    int is_student;
    char courses[3][30]; // 假设最多有3门课程
    char street[100];
    char city[50];
    char zip[10];
} Person;

// 记录内存分配的结构
typedef struct {
    size_t total_allocated;
    size_t total_freed;
} MemoryTracker;


// 定义支持的元组元素类型
typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STRING
} ElementType;


// 定义元组元素结构
typedef struct {
    ElementType type;
    union {
        int int_val;
        float float_val;
        char* string_val;
    } data;
} TupleElement;

// 定义元组结构
typedef struct {
    TupleElement* elements;
    size_t size;
} Tuple;



extern MemoryTracker mem_tracker; // 声明内存跟踪器

// 重写 malloc 和 free 以跟踪内存使用
void* tracked_malloc(size_t size);
void tracked_free(void* ptr, size_t size);

// 测试函数
void test_function();

// JSON 解析函数声明
void parse_json(const char *filename);

// 更新 JSON 文件中所有 Person 对象的函数
void update_persons(const char *filename);
/****************************************************/
// 函数声明
Tuple* tuple_create(size_t size);
void tuple_set_element(Tuple* tuple, size_t index, TupleElement element);
TupleElement tuple_get_element(const Tuple* tuple, size_t index);
void tuple_print(const Tuple* tuple);
void tuple_free(Tuple* tuple);
/****************************************************/
void test_Tuple();

void move_elements(List *l, List *lb, size_t i, size_t len);


#endif // COMMON_H  