/*
 * @Author: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @Date: 2024-11-25 00:22:17
 * @LastEditors: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @LastEditTime: 2024-11-28 01:03:02
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

#include "math/subtract.h"  // 添加新的头文件
#include "vector/vector.h"  // 添加新的头文件
#include "list/list.h"  // 添加新的头文件
#include "mempool/mempool.h"  // 添加新的头文件
#include "test_leetecode/test_leetecode.h"
#include "mapset/mapset.h"
#include "cjson/cJSON.h"

#include "mapset/hashmap.h"
#include <math.h>
#include "common.h"
#define EPSILON 1e-6
#define NUM_PERSONS 30
#include "ValueRange/ValueRange.h"






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


static void range_value();
static void range_value()
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


int main(int argc, char **argv) {
    // 设置区域为 UTF-8
    setlocale(LC_ALL, "");

    // printf("Hello, World!\n");
    // printf("The sum of 3 and 5 is: %d\n", add(3, 5));
    // printf("The difference of 10 and 4 is: %d\n", subtract(10, 4));
    // printf("The product of 3 and 5 is: %d\n", multiply(3, 5));  // 使用新的乘法函数



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
        printf("Value for 'key1': %d\n", value1);
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






    //test_stack();


    //test_Tuple();

    range_value();

    return 0;
}
