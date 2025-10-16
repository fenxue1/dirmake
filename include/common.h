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

#include "../src/cjson/cJSON.h"
#include "../src/list/list.h"
#include "../src/mempool/mempool.h"
#include "../src/test_leetecode/test_leetecode.h"
#include "../src/mapset/mapset.h"
#include "../src/mapset/hashmap.h"
#include "../src/gobject/gobject.h"
#include "tr_text.h"

// 定义 Person 结构体
typedef struct {
    int id;
    char name[50];
    int age;
    int is_student;
    char courses[3][30];
    char street[100];
    char city[50];
    char zip[10];
    float score;
} Person;

// 基于GObject的Person_common类
typedef struct {
    GObject parent;
    
    char *name;
    int age;
    char *email;
} Person_common;

// Person_common的类结构
typedef struct {
    GObjectClass parent_class;
    
    // 虚函数
    void (*say_hello)(Person_common *self);
    void (*celebrate_birthday)(Person_common *self);
} Person_commonClass;

// 内存跟踪器结构
typedef struct {
    size_t total_allocated;
    size_t total_freed;
} MemoryTracker;

// 元组元素类型
typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STRING
} ElementType;

// 元组元素结构
typedef struct {
    ElementType type;
    union {
        int int_val;
        float float_val;
        char* string_val;
    } data;
} TupleElement;

// 元组结构
typedef struct {
    TupleElement* elements;
    size_t size;
} Tuple;

// I2S配置结构
typedef struct{
    int bck_io_num;
    int ws_io_num;
    int data_out_num;
    int data_in_num;
} i2s_pin_config_t;

extern MemoryTracker mem_tracker;

// 内存跟踪函数
void* tracked_malloc(size_t size);
void tracked_free(void* ptr, size_t size);

// 测试函数
void test_function();

// JSON处理函数
void parse_json(const char *filename);
void update_persons(const char *filename);

// 元组函数
Tuple* tuple_create(size_t size);
void tuple_set_element(Tuple* tuple, size_t index, TupleElement element);
TupleElement tuple_get_element(const Tuple* tuple, size_t index);
void tuple_print(const Tuple* tuple);
void tuple_free(Tuple* tuple);

void test_Tuple();
int game_test();

// Person_common类型系统
g_type_t person_common_get_type(void);
#define PERSON_COMMON_TYPE (person_common_get_type())
#define PERSON_COMMON(obj) ((Person_common*)(obj))
#define PERSON_COMMON_CLASS(klass) ((Person_commonClass*)(klass))

// 信号名称
#define PERSON_SIGNAL_BIRTHDAY "birthday"
#define PERSON_SIGNAL_NAME_CHANGED "name-changed"

// Person_common公共API
Person_common* person_common_new(const char *name, int age);
void person_common_say_hello(Person_common *person);
void person_common_celebrate_birthday(Person_common *person);
void person_common_set_email(Person_common *person, const char *email);
const char* person_common_get_email(Person_common *person);

// 类初始化函数（不再是static）
void person_common_class_init(Person_commonClass *klass);
void person_common_init(Person_common *self);

// 测试函数
void test_person_common(void);

#endif // COMMON_H



/*
  编写一个Point2D的使用例子，从命令行介绍一个整数N，在正方型中成N个随机点，随后计算两个点之间的最近距离


*/
