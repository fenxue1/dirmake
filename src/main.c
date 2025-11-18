/*
 * @Author: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @Date: 2024-11-25 00:22:17
 * @LastEditors: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @LastEditTime: 2025-10-08 23:00:24
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

#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "threadpool/threadpool.h"
#include "utils/utils.h"
#include "math/multiply.h"   // 添加新的头文件
#include "math/subtract.h"   // 添加新的头文件
#include "vector/vector.h"   // 添加新的头文件
#include "list/list.h"       // 添加新的头文件
#include "mempool/mempool.h" // 添加新的头文件
#include "test_leetecode/test_leetecode.h"
#include "mapset/mapset.h"
#include "cjson/cJSON.h"
#include "ValueRange/ValueRange.h"
#include "tr_text.h"

static const _Tr_TEXT txt_input_points_4444= {
    "输入点",
    "Input Points",
    "\x53\x65\x6c\x65\x63\x63\x69\xc3\xb3\x6e\x20\x64\x65\x20\x63\xc3\xb3\x64\x69\x67\x6f",
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
#include "mapset/hashmap.h"
#include <math.h>
#include "common.h"
#include <stdint.h>
#include <string.h>

#include "gobject/gobject.h"

#define EPSILON 1e-6
#define NUM_PERSONS 30


#define MAX(a, b) \
    ({ \
        typeof(a) _a = (a); \
        typeof(b) _b = (b); \
        (void) (&_a == &_b); \
        ((_a)>(_b) ? (_a) : (_b)); \
    })

// 比较函数类型
typedef int (*compare_func)(void *, void *);

// 比较两个整数的函数
int compare_int(void *a, void *b)
{
    return (*(int *)a - *(int *)b);
}

// 比较两个 double 的函数
int compare_double(void *a, void *b)
{
    double diff = *(double *)a - *(double *)b;
    return (diff > 0) - (diff < 0); // 返回 1, 0, -1
}
//http://ckapi.sevenbrothers.cn/bili/api?id=BV1HWNHevEgG
// Shell 排序实现
void shell_sort(Vector *vector, compare_func cmp)
{
    size_t N = vector->size;
    int h = 1;
    while (h < N / 3)
        h = 3 * h + 1; // 1, 4, 13, 40, 121, 364, 1093, ...
    while (h >= 1)
    {
        for (size_t i = h; i < N; i++)
        {
            for (size_t j = i; j >= h && cmp(vector->data[j], vector->data[j - h]) < 0; j -= h)
            {
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
    Stack *stack = create_stack();

    int a = 10, b = 20, c = 30;
    stack_push(stack, &a);
    stack_push(stack, &b);
    stack_push(stack, &c);

    printf("Top of stack: %d\n", *(int *)stack_peek(stack));

    int *value = (int *)stack_pop(stack);
    printf("Popped: %d\n", *value);

    value = (int *)stack_pop(stack);
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
    for (size_t i = 0; i < count; i++)
    {
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
    Deque *deque = create_deque();

    int values[] = {1, 2, 3, 4, 5};

    // 从前端插入
    for (int i = 0; i < 5; i++)
    {
        deque_push_front(deque, &values[i]);
    }

    // 从后端插入
    for (int i = 6; i <= 10; i++)
    {
        deque_push_back(deque, &i);
    }

    // 打印前端元素
    printf("Front: %d\n", *(int *)deque_front(deque)); // 应该是5

    // 打印后端元素
    printf("Back: %d\n", *(int *)deque_back(deque)); // 应该是10

    // 从前端删除元素
    printf("Popped from front: %d\n", *(int *)deque_pop_front(deque)); // 应该是5

    // 从后端删除元素
    printf("Popped from back: %d\n", *(int *)deque_pop_back(deque)); // 应该是10

    free_deque(deque);
}

void decode_hex_string(const char *hex_str)
{
    int len = strlen(hex_str);
    char decoded_str[len / 4 + 1];

    for (int i = 0, j = 0; i < len; i += 4, j++)
    {
        sscanf(hex_str + i, "\\x%2hhX", &decoded_str[j]);
    }

    decoded_str[len / 4] = '\0';

    printf("Decoded string: %s\n", decoded_str);
}

// 交换函数
void test_swap(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

int MaxProduct2(Vector *vec, int max)
{
    int max_a = 0;

    for (size_t i = 0; i < vec->size; i++)
    {
        if (vec->data[i] > max_a)
        {
            max_a = vec->data[i];
        }
    }
    max_a = max_a * max;
    return max_a;
}

/**
 * 安全拼接两个字符串，用 "|" 分隔，结果存入静态缓冲区
 *
 * @param dest     目标缓冲区
 * @param src1     第一个源字符串
 * @param src2     第二个源字符串
 * @param dest_size 目标缓冲区总大小（包括终止符）
 *
 * @return 0 成功，-1 缓冲区不足
 */
int32_t embedded_strcat_pipe_safe(
    char *dest,
    const char *src1,
    const char *src2,
    size_t dest_size)
{
    if (dest == NULL || src1 == NULL || src2 == NULL || dest_size == 0)
    {
        return -1; // 参数无效
    }

    size_t src1_len = strlen(src1);
    size_t src2_len = strlen(src2);
    size_t total_needed = src1_len + 1 + src2_len + 1; // 包括分隔符和终止符

    if (total_needed > dest_size)
    {
        return -1; // 缓冲区不足
    }

    // 复制 src1
    memcpy(dest, src1, src1_len);
    dest += src1_len;

    // 添加分隔符
    *dest = '|';
    dest++;

    // 复制 src2
    memcpy(dest, src2, src2_len);
    dest += src2_len;

    // 终止符
    *dest = '\0';

    return 0;
}

/**
 * 轻量级拼接函数，用 "|" 分隔
 *
 * @param dest     目标缓冲区（需足够大）
 * @param src1     第一个源字符串
 * @param src2     第二个源字符串
 *
 * @return 写入的字符数（不含终止符），若缓冲区不足则返回 -1
 */
int32_t embedded_strcat_pipe_light(
    char *dest,
    const char *src1,
    const char *src2,
    size_t dest_size)
{
    if (dest == NULL || src1 == NULL || src2 == NULL || dest_size == 0)
    {
        return -1;
    }

    char *ptr = dest;
    size_t remaining = dest_size - 1; // 保留终止符空间

    // 复制 src1
    while (*src1 != '\0' && remaining > 0)
    {
        *ptr++ = *src1++;
        remaining--;
    }

    // 检查是否复制完 src1
    if (*src1 != '\0')
    {
        return -1; // 缓冲区不足
    }

    // 添加分隔符
    if (remaining == 0)
        return -1;
    *ptr++ = '|';
    remaining--;

    // 复制 src2
    while (*src2 != '\0' && remaining > 0)
    {
        *ptr++ = *src2++;
        remaining--;
    }

    // 检查是否复制完 src2
    if (*src2 != '\0')
    {
        return -1; // 缓冲区不足
    }

    // 终止符
    *ptr = '\0';

    return (ptr - dest); // 返回实际写入长度
}

static void test_string(void)
{

    char buffer[32]; // 静态分配，大小明确
    const char *str1 = "这是可以测试";
    const char *str2 = "Value100";

    if (embedded_strcat_pipe_safe(buffer, str1, str2, sizeof(buffer)) == 0)
    {
        // 成功：buffer = "Sensor1|Value100"
        printf("------%s\n", buffer);
    }
    else
    {
        // 处理错误（如缓冲区不足）
    }

    int32_t written = embedded_strcat_pipe_light(buffer, str1, str2, sizeof(buffer));
    if (written > 0)
    {
        printf("------%s---------%s\n", str1, str2);
    }
    else
    {
        // 处理错误（如缓冲区不足）
    }
}
// 内存池性能测试
void test_memory_pool_performance()
{
    printf("============内存池性能测试====================");

    // 创建一个内容量的而链表
    CList *list = clist_new(sizeof(int), 1000);
    printf("创建容量为1000的纯数2链表\n");
    printf("初始状态: count = %d,allSize = %d bytes\n", clist_count(list), clist_allSize(list));

    // 批量添加元素
    printf("\n批量添加个元素..\n");
    for (int i = 0; i < 500; i++)
    {
        clist_add(list, &i);
    }
    printf("添加后状态: count = %d,allSize = %d bytes\n", clist_count(list), clist_allSize(list));
    // 批量删除元素
    printf("\n批量删除500个元素..\n");
    for (int i = 0; i < clist_count(list); i += 2)
    {
        if (i < clist_count(list))
        {
            clist_remove(list, i); // 应为
            i--;                   // 因为删除因为删除索引会变化
        }
    }
    printf("删除后状态: count = %d,allSize = %d bytes\n", clist_count(list), clist_allSize(list));

    // 再词添加元素，测试空闲的重用
    printf("\n再添加100个元素..(测试槽位重用)\n");
    for (int i = 1000; i < 1100; i++)
    {
        clist_add(list, &i);
    }
    printf("添加后状态: count = %d,allSize = %d bytes\n", clist_count(list), clist_allSize(list));
}

// 内存碎片化测试
void test_memory_fragmentation()
{
    printf("========== 内存碎片化测试 ==========\n");

    CList *list = clist_new(sizeof(int), 20);

    // 添加20个元素
    printf("添加20个元素...\n");
    for (int i = 0; i < 20; i++)
    {
        clist_add(list, &i);
    }

    // 删除奇数位置的元素，造成碎片化
    printf("删除奇数位置的元素（造成碎片化）...\n");
    for (int i = 1; i < 20; i += 2)
    {
        clist_remove(list, i);
        i--; // 调整索引
    }

    printf("碎片化后状态:\n");
    clist_print(list, 0, -1, "int");

    // 添加新元素，查看是否重用了空闲槽位
    printf("\n添加新元素100-104，观察槽位重用:\n");
    for (int i = 100; i < 105; i++)
    {
        clist_add(list, &i);
    }

    clist_print(list, 0, -1, "int");

    clist_free(list);
    printf("\n碎片化测试完成\n\n");
}



// 信号处理函数
static void on_birthday(GObject *object, void *data) {
    Person_common *person = PERSON_COMMON(object);
    printf("🎂 Birthday signal received for %s!\n", person->name);
}

static void on_name_changed(GObject *object, void *data) {
    Person_common *person = PERSON_COMMON(object);
    printf("📝 Name changed signal received for %s\n", person->name);
}


void show(int a, float b, float m)
{
    printf("a = %d, b = %f, m = %f\n", a, b, m);
}

int main(int argc, char **argv)
{
    // 设置区域为 UTF-8
    setlocale(LC_ALL, "utf=8");

/*


   // test_person_common();
    //     test_string();

    //     // printf("Hello, World!\n");
    //     // printf("The sum of 3 and 5 is: %d\n", add(3, 5));
    //     // printf("The difference of 10 and 4 is: %d\n", subtract(10, 4));
    //     // printf("The product of 3 and 5 is: %d\n", multiply(3, 5));  // 使用新的乘法函数

    //    char str[] = "どんなテキストでもいいですか。";

    //     for(int i=0; i<strlen(str); i++) {
    //         printf("\\x%02X", (unsigned char)str[i]);
    //     }

    //      const char* hex_str = "\\xE3\\x81\\A9\\xE3\\x82\\x93\\xE3\\x81\\AA\\xE3\\x83\\x86\\xE3\\x82\\xAD\\xE3\\x82\\xB9\\xE3\\x83\\x88\\xE3\\x81\\xA7\\xE3\\x82\\x82\\xE3\\x81\\x84\\xE3\\x81\\x84\\xE3\\x81\\xA7\\xE3\\x81\\x99\\xE3\\x81\\x8B\\xE3\\x80\\x82";
    //      decode_hex_string(hex_str);

    //     test_stack();

    //     MapSet *mapset = mapset_create(10);
    //     if (!mapset) {
    //         fprintf(stderr, "Failed to create MapSet.\n");
    //         return 1;
    //     }

    //     mapset_insert(mapset, "apple", 1);
    //     mapset_insert(mapset, "banana", 2);
    //     mapset_insert(mapset, "orange", 3);

    //     int value;
    //     if (mapset_find(mapset, "banana", &value) == 0) {
    //         printf("Value for 'banana': %d\n", value);
    //     } else {
    //         printf("'banana' not found.\n");
    //     }

    //     mapset_remove(mapset, "apple");

    //     if (mapset_find(mapset, "apple", &value) == 0) {
    //         printf("Value for 'apple': %d\n", value);
    //     } else {
    //         printf("'apple' not found.\n");
    //     }

    //     mapset_destroy(mapset);

    //       // Create a new hash map
    //     HashMap *map = create_hashmap();
    //     if (!map) {
    //         fprintf(stderr, "Failed to create hash map\n");
    //         return EXIT_FAILURE;
    //     }

    //     // Insert key-value pairs into the hash map
    //     hashmap_put(map, "key1", 100);
    //     hashmap_put(map, "key2", 200);
    //     hashmap_put(map, "key3", 300);

    //     // Retrieve and print values from the hash map
    //     int value1 = hashmap_get(map, "key1");
    //     if (value1 != -1) {
    //         printf("Value for 中午跟'key1': %d\n", value1);
    //     } else {
    //         printf("'key1' not found\n");
    //     }

    //     value1 = hashmap_get(map, "key2");
    //     if (value1 != -1) {
    //         printf("Value for 'key2': %d\n", value1);
    //     } else {
    //         printf("'key2' not found\n");
    //     }

    //     value1 = hashmap_get(map, "key3");
    //     if (value1 != -1) {
    //         printf("Value for 'key3': %d\n", value1);
    //     } else {
    //         printf("'key3' not found\n");
    //     }

    //     // Remove a key-value pair
    //     hashmap_remove(map, "key2");

    //     // Try to retrieve the removed key
    //     value1 = hashmap_get(map, "key2");
    //     if (value1 != -1) {
    //         printf("Value for 'key2': %d\n", value1);
    //     } else {
    //         printf("'key2' not found after removal\n");
    //     }

    //     // Clean up and free the hash map
    //     destroy_hashmap(map);

    //      srand(time(NULL));  // 初始化随机数种子

    //     SkipList *slist = create_skiplist();

    //     // 插入元素
    //     SkipList_insert(slist, 3);
    //     SkipList_insert(slist, 6);
    //     SkipList_insert(slist, 7);
    //     SkipList_insert(slist, 9);
    //     SkipList_insert(slist, 12);
    //     SkipList_insert(slist, 19);
    //     SkipList_insert(slist, 17);
    //     SkipList_insert(slist, 26);
    //     SkipList_insert(slist, 21);
    //     SkipList_insert(slist, 25);

    //      // 查找元素
    //     Node *found = SkipList_search(slist, 19);
    //     if (found) {
    //         printf("Found: %d\n", found->key);
    //     } else {
    //         printf("Not found: 19\n");
    //     }

    //       Vector *vector = create_vector(4);
    //     if (!vector) return 1;

    //     // 添加 double 到 Vector
    //     double a = 10.5;
    //     double b = 5.2;
    //     double c = 20.8;
    //     double d = 15.3;

    //     vector_push_back(vector, &a);
    //     vector_push_back(vector, &b);
    //     vector_push_back(vector, &c);
    //     vector_push_back(vector, &d);

    //     // 执行 Shell 排序
    //     shell_sort(vector, compare_double);//aaa

    //     // 打印排序后的结果
    //     for (size_t i = 0; i < vector->size; i++) {
    //         printf("%.2f ", *(double*)vector->data[i]);
    //     }
    //     printf("\n");

    //     // 释放 Vector
    //     free_vector(vector);

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

    // test_stack();

    // test_Tuple();

    //     CTree* tree = (CTree*)malloc(sizeof(CTree));
    //     tree->n = 0; // 初始化节点数量
    //     tree->r = 0; // 假设根节点为0

    //     // 示例：添加节点
    //     tree->nodes[0].data = 1; // 根节点
    //     tree->nodes[0].firstchild = NULL; // 初始化孩子指针
    //     tree->n++;

    //     tree->nodes[1].data = 2; // 添加第一个子节点
    //     tree->nodes[1].firstchild = NULL; // 初始化孩子指针
    //     tree->n++;
    //     addChild(tree, 0, 1); // 将节点1添加为节点0的子节点

    //     tree->nodes[2].data = 3; // 添加第二个子节点
    //     tree->nodes[2].firstchild = NULL; // 初始化孩子指针
    //     tree->n++;
    //     addChild(tree, 0, 2); // 将节点2添加为节点0的子节点

    //     tree->nodes[3].data = 4; // 添加第三个子节点
    //     tree->nodes[3].firstchild = NULL; // 初始化孩子指针
    //     tree->n++;
    //     addChild(tree, 1, 3); // 将节点3添加为节点1的子节点

    //     // 打印树的结构和每个节点的度
    //     printTreeAndDegrees(tree);

    //     // 释放内存
    //     for (int i = 0; i < tree->n; i++) {
    //         CTNode* child = tree->nodes[i].firstchild;
    //         while (child != NULL) {
    //             CTNode* temp = child;
    //             child = child->next;
    //             free(temp);
    //         }
    //     }
    //     free(tree);

    //     Vector *vec = create_vector(10);
    //     int array[] = {26, 30, 3, 53, 32, 5, 34, 33, 43, 2};

    //     for (size_t i = 0; i < 10; i++) {
    //         vector_push_back(vec, &array[i]);
    //     }

    //     for (size_t i = 0; i < vec->size; i++) {
    //         printf("%d \n----", *(int*)vec->data[i]);
    //     }

    //     MaxProduct2(vec,100);
    //    //便利这个vec
    //     shell_sort(vec, compare_int);//aaa
    //     for (size_t i = 0; i < vec->size; i++) {
    //         printf("%d \n", *(int*)vec->data[i]);
    //     }
    //     printf("\n");

    //     printf("%d \n",MaxProduct2(vec,20));
    //     free_vector(vec);
    
    // printf("========== CList 内存池版本示例 ==========\n\n");

    // // ================= 基本功能测试 =================
    // printf("=== 基本功能测试 ===\n");

    // // 创建链表时指定初始容量
    // CList *int_list = clist_new(sizeof(int), 10);
    // if (!int_list)
    // {
    //     printf("Failed to create list\n");
    //     return 1;
    // }

    // printf("创建初始容量为10的整数链表\n");
    // printf("初始状态: count=%d, allSize=%d bytes\n",
    //        clist_count(int_list), clist_allSize(int_list));

    // // 添加元素
    // printf("\n添加元素 1-7: ");
    // for (int i = 1; i <= 7; i++)
    // {
    //     int *added = (int *)clist_add(int_list, &i);
    //     printf("%d ", *added);
    // }
    // printf("\n");

    // printf("链表内容和槽位分布:\n");
    // clist_print(int_list, 0, -1, "int");

    // // 删除中间元素
    // printf("\n删除索引2和4的元素...\n");
    // clist_remove(int_list, 4); // 先删除索引大的
    // clist_remove(int_list, 2);

    // printf("删除后的内容:\n");
    // clist_print(int_list, 0, -1, "int");

    // // 添加新元素，观察槽位重用
    // printf("\n添加新元素80和90:\n");
    // int val80 = 80, val90 = 90;
    // clist_add(int_list, &val80);
    // clist_add(int_list, &val90);

    // printf("添加后的内容（注意槽位重用）:\n");
    // clist_print(int_list, 0, -1, "int");

    // // ================= 结构体测试 =================
    // printf("\n=== 结构体测试 ===\n");

    // CList *person_list = clist_new(sizeof(Person), 5);

    // Person persons[] = {
    //     {1, "Alice", 95.5f},
    //     {2, "Bob", 87.0f},
    //     {3, "Charlie", 92.3f}};

    // printf("添加人员信息:\n");
    // for (int i = 0; i < 3; i++)
    // {
    //     Person *added = (Person *)clist_add(person_list, &persons[i]);
    //     printf("添加: ID=%d, Name=%s, Score=%.1f\n",
    //            added->id, added->name, added->score);
    // }

    // printf("\n-------------------人员列表详情:\n");

    // for (int i = 0; i < clist_count(person_list); i++)
    // {
    //     Person *p = (Person *)clist_at(person_list, i);
    //     // printf("[%d] ID=%d, Name=%-10s, Score=%.1f\n",
    //     //        i, p->id, p->name, p->score);

    //     printf("[%d] \n",
    //            i);
    // }

    // // 安全地访问第一个元素
    // Person *pp = (Person *)person_list->at(person_list, 2);
    // if (pp != NULL)
    // {
    //     printf("First person: ID=%d, Name=%s\n Score=%.1f\n", pp->id, pp->name, pp->score);
    // }
    // else
    // {
    //     printf("Failed to get first person - returned NULL\n");
    // }

    // // 测试查找功能
    // printf("\n查找测试:\n");
    // int search_id = 2;
    // Person *found = (Person *)clist_firstMatch(person_list, &search_id, 0);
    // if (found)
    // {
    //     printf("找到ID为2的人员: %s, Score=%.1f\n", found->name, found->score);
    // }

    // // ================= 容量管理测试 =================
    // printf("\n=== 容量管理测试 ===\n");

    // printf("当前容量信息: count=%d, allSize=%d bytes\n",
    //        clist_count(int_list), clist_allSize(int_list));

    // printf("手动调整容量到20...\n");
    // clist_realloc(int_list, 20);
    // printf("调整后: count=%d, allSize=%d bytes\n",
    //        clist_count(int_list), clist_allSize(int_list));

    // printf("添加更多元素触发自动扩容...\n");
    // for (int i = 100; i < 115; i++)
    // {
    //     clist_add(int_list, &i);
    // }
    // printf("自动扩容后: count=%d, allSize=%d bytes\n",
    //        clist_count(int_list), clist_allSize(int_list));

    // // ================= 性能测试 =================
    // test_memory_pool_performance();
    // test_memory_fragmentation();

    // // ================= 清理和释放 =================
    // printf("=== 资源清理 ===\n");

    // printf("清空int_list...\n");
    // clist_clear(int_list);
    // printf("清空后: count=%d, allSize=%d bytes\n",
    //        clist_count(int_list), clist_allSize(int_list));

    // printf("释放所有链表...\n");
    // clist_free(int_list);
    // clist_free(person_list);

    // printf("程序运行完成\n");
*/
    
    int a = 10;
    float b = 20.0f;
    float m = MAX(a, b);
    show(a, b, m);


    return 0;
}
