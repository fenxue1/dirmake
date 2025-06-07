/*
 * @Author: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @Date: 2024-11-25 00:22:17
 * @LastEditors: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @LastEditTime: 2024-12-05 21:32:24
 * @FilePath: \test_cmake\src\main.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/*
 * @Author: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @Date: 2024-11-17 22:45:01
 * @LastEditors: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @LastEditTime: 2024-11-25 19:16:52
 * @FilePath: \test_cmake\src\main.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <locale.h>
#include <locale.h>
#include <time.h>
#include <stdbool.h>

#include "threadpool/threadpool.h"   
#include "utils/utils.h"
#include "math/multiply.h"  // 添加新的头文件
#include "math/subtract.h"  // 添加新的头文件
#include "vector/vector.h"  // 添加新的头文件
#include "list/list.h"  // 添加新的头文件
#include "mempool/mempool.h"  // 添加新的头文件
#include "test_leetecode/test_leetecode.h"
#include "mapset/mapset.h"
#include "cjson/cJSON.h"
#include "ValueRange/ValueRange.h"
#include "mapset/hashmap.h"
#include <math.h>
#include "common.h"
#define EPSILON 1e-6
#define NUM_PERSONS 30
// 比较函数类型
typedef int (*compare_func)(void*, void*);


// 比较两个整数的函数
int compare_int(void *a, void *b) {
    return (*(int*)a - *(int*)b);
}


// 比较两个 double 的函数
int compare_double(void *a, void *b) {
    double diff = *(double*)a - *(double*)b;
    return (diff > 0) - (diff < 0); // 返回 1, 0, -1
}


// Shell 排序实现
void shell_sort(Vector *vector, compare_func cmp) {
    size_t N = vector->size;
    int h = 1;
    while (h < N / 3) h = 3 * h + 1; // 1, 4, 13, 40, 121, 364, 1093, ...
    while (h >= 1) {
        for (size_t i = h; i < N; i++) {
            for (size_t j = i; j >= h && cmp(vector->data[j], vector->data[j - h]) < 0; j -= h) {
                // 交换元素
                void *temp = vector->data[j];
                vector->data[j] = vector->data[j - h];
                vector->data[j - h] = temp;
            }
        }
        h = h / 3;
    }
}


void test_stack()
{
     Stack* stack = create_stack();

    int a = 10, b = 20, c = 30;
    stack_push(stack, &a);
    stack_push(stack, &b);
    stack_push(stack, &c);

    printf("Top of stack: %d\n", *(int*)stack_peek(stack));

    int* value = (int*)stack_pop(stack);
    printf("Popped: %d\n", *value);

    value = (int*)stack_pop(stack);
    printf("Popped: %d\n", *value);

    free_stack(stack);


}


static void range_value(void);
static void range_value(void)
{

     const size_t count = 3;
    ValueRange vr_array[count];

    double min[] = {0.0, 10.0, 20.0};
    double max[] = {100.0, 200.0, 300.0};
    double initial[] = {50.0, 150.0, 250.0};
    double step[] = {5.0, 10.0, 15.0};

    // 初始化 ValueRange 数组
    init_value_range_array(vr_array, count, min, max, initial, step);

    // 显示每个 ValueRange 的信息
    for (size_t i = 0; i < count; i++) {
        vr_array[i].display(&vr_array[i]);
    }

    // 增加和减少值
    vr_array[0].increment(&vr_array[0]);
    vr_array[0].display(&vr_array[0]);

    vr_array[1].decrement(&vr_array[1]);
    vr_array[1].display(&vr_array[1]);
}


void set_Deque()
{
     Deque* deque = create_deque();

    int values[] = {1, 2, 3, 4, 5};

    // 从前端插入
    for (int i = 0; i < 5; i++) {
        deque_push_front(deque, &values[i]);
    }

    // 从后端插入
    for (int i = 6; i <= 10; i++) {
        deque_push_back(deque, &i);
    }

    // 打印前端元素
    printf("Front: %d\n", *(int*)deque_front(deque)); // 应该是5

    // 打印后端元素
    printf("Back: %d\n", *(int*)deque_back(deque)); // 应该是10

    // 从前端删除元素
    printf("Popped from front: %d\n", *(int*)deque_pop_front(deque)); // 应该是5

    // 从后端删除元素
    printf("Popped from back: %d\n", *(int*)deque_pop_back(deque)); // 应该是10

    free_deque(deque);


}



void decode_hex_string(const char* hex_str) {
    int len = strlen(hex_str);
    char decoded_str[len/4 + 1];
    
    for (int i = 0, j = 0; i < len; i += 4, j++) {
        sscanf(hex_str + i, "\\x%2hhX", &decoded_str[j]);
    }
    
    decoded_str[len/4] = '\0';
    
    printf("Decoded string: %s\n", decoded_str);
}





int main(int argc, char **argv) {
    // 设置区域为 UTF-8
    setlocale(LC_ALL, "utf=8");

    // printf("Hello, World!\n");
    // printf("The sum of 3 and 5 is: %d\n", add(3, 5));
    // printf("The difference of 10 and 4 is: %d\n", subtract(10, 4));
    // printf("The product of 3 and 5 is: %d\n", multiply(3, 5));  // 使用新的乘法函数

   char str[] = "どんなテキストでもいいですか。";
    
    for(int i=0; i<strlen(str); i++) {
        printf("\\x%02X", (unsigned char)str[i]);
    }

     const char* hex_str = "\\xE3\\x81\\A9\\xE3\\x82\\x93\\xE3\\x81\\AA\\xE3\\x83\\x86\\xE3\\x82\\xAD\\xE3\\x82\\xB9\\xE3\\x83\\x88\\xE3\\x81\\xA7\\xE3\\x82\\x82\\xE3\\x81\\x84\\xE3\\x81\\x84\\xE3\\x81\\xA7\\xE3\\x81\\x99\\xE3\\x81\\x8B\\xE3\\x80\\x82";
     decode_hex_string(hex_str);
    

    test_stack();

    MapSet *mapset = mapset_create(10);
    if (!mapset) {
        fprintf(stderr, "Failed to create MapSet.\n");
        return 1;
    }

    mapset_insert(mapset, "apple", 1);
    mapset_insert(mapset, "banana", 2);
    mapset_insert(mapset, "orange", 3);

    int value;
    if (mapset_find(mapset, "banana", &value) == 0) {
        printf("Value for 'banana': %d\n", value);
    } else {
        printf("'banana' not found.\n");
    }

    mapset_remove(mapset, "apple");

    if (mapset_find(mapset, "apple", &value) == 0) {
        printf("Value for 'apple': %d\n", value);
    } else {
        printf("'apple' not found.\n");
    }

    mapset_destroy(mapset);


      // Create a new hash map
    HashMap *map = create_hashmap();
    if (!map) {
        fprintf(stderr, "Failed to create hash map\n");
        return EXIT_FAILURE;
    }

    // Insert key-value pairs into the hash map
    hashmap_put(map, "key1", 100);
    hashmap_put(map, "key2", 200);
    hashmap_put(map, "key3", 300);

    // Retrieve and print values from the hash map
    int value1 = hashmap_get(map, "key1");
    if (value1 != -1) {
        printf("Value for 中午跟'key1': %d\n", value1);
    } else {
        printf("'key1' not found\n");
    }

    value1 = hashmap_get(map, "key2");
    if (value1 != -1) {
        printf("Value for 'key2': %d\n", value1);
    } else {
        printf("'key2' not found\n");
    }

    value1 = hashmap_get(map, "key3");
    if (value1 != -1) {
        printf("Value for 'key3': %d\n", value1);
    } else {
        printf("'key3' not found\n");
    }

    // Remove a key-value pair
    hashmap_remove(map, "key2");

    // Try to retrieve the removed key
    value1 = hashmap_get(map, "key2");
    if (value1 != -1) {
        printf("Value for 'key2': %d\n", value1);
    } else {
        printf("'key2' not found after removal\n");
    }

    // Clean up and free the hash map
    destroy_hashmap(map);


     srand(time(NULL));  // 初始化随机数种子

    SkipList *slist = create_skiplist();

    // 插入元素
    SkipList_insert(slist, 3);
    SkipList_insert(slist, 6);
    SkipList_insert(slist, 7);
    SkipList_insert(slist, 9);
    SkipList_insert(slist, 12);
    SkipList_insert(slist, 19);
    SkipList_insert(slist, 17);
    SkipList_insert(slist, 26);
    SkipList_insert(slist, 21);
    SkipList_insert(slist, 25);


     // 查找元素
    Node *found = SkipList_search(slist, 19);
    if (found) {
        printf("Found: %d\n", found->key);
    } else {
        printf("Not found: 19\n");
    }



      Vector *vector = create_vector(4);
    if (!vector) return 1;

    // 添加 double 到 Vector
    double a = 10.5;
    double b = 5.2;
    double c = 20.8;
    double d = 15.3;

    vector_push_back(vector, &a);
    vector_push_back(vector, &b);
    vector_push_back(vector, &c);
    vector_push_back(vector, &d);

    // 执行 Shell 排序
    shell_sort(vector, compare_double);//aaa

    // 打印排序后的结果
    for (size_t i = 0; i < vector->size; i++) {
        printf("%.2f ", *(double*)vector->data[i]);
    }
    printf("\n");

    // 释放 Vector
    free_vector(vector);

    //  // 创建 CarBuilder
    CarBuilder* builder = createCarBuilder();

    //  // 设置品牌
    builder->setBrand(builder, "Toyota");
    //  // 设置型号
    builder->setModel(builder, "Corolla");
    //  // 设置颜色
    builder->setColor(builder, "Blue");
    //
    builder->setYear(builder, 2022);

    builder->reset(builder);

    builder->setModel(builder, "Camry");  
    builder->setColor(builder, "Red");
    builder->setBrand(builder, "Honda");
    builder->setYear(builder, 2023);
    //  // 构建 Car
    Car* myCar = builder->build(builder);

    //  // 打印 Car 的信息
    printf("Brand: %s\n", myCar->brand);
    printf("Model: %s\n", myCar->model);
    printf("Color: %s\n", myCar->color);
    printf("Year: %d\n", myCar->year);

    Car* myCar2 = builder->build(builder);
    
    //  // 打印 Car 的信息
    printf("Brand: %s\n", myCar2->brand);
    printf("Model: %s\n", myCar2->model);
    printf("Color: %s\n", myCar2->color);
    printf("Year: %d\n", myCar2->year);

    builder->reset(builder);
    free(builder->car);
    free(builder);

    //  // 删除元素
    // SkipList_delete(slist, 19);
    // found = SkipList_search(slist, 19);
    // if (found) {
    //     printf("Found: %d\n", found->key);
    // } else {
    //     printf("Not found: 19\n");
    // }

    
    // // 销毁跳表
    // destroy_skiplist(slist);




    //  // 创建并初始化 30 个 Person 对象
    // Person persons[NUM_PERSONS];
    // for (size_t i = 0; i < NUM_PERSONS; i++) {
    //     snprintf(persons[i].name, sizeof(persons[i].name), "Person %zu", i + 1);
    //     persons[i].age = 20 + (i % 10); // 年龄在 20 到 29 之间
    //     persons[i].is_student = (i % 2); // 偶数为学生，奇数为非学生
    //     snprintf(persons[i].courses[0], sizeof(persons[i].courses[0]), "Course A");
    //     snprintf(persons[i].courses[1], sizeof(persons[i].courses[1]), "Course B");
    //     snprintf(persons[i].courses[2], sizeof(persons[i].courses[2]), "Course C");
    //     snprintf(persons[i].street, sizeof(persons[i].street), "Street %zu", i + 1);
    //     snprintf(persons[i].city, sizeof(persons[i].city), "City %zu", i + 1);
    //     snprintf(persons[i].zip, sizeof(persons[i].zip), "Zip %zu", i + 1);
    // }



   // range_value();


    //test_stack();


    //test_Tuple();



    return 0;
}
