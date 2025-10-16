#include "../include/common.h"
#include "tr_text.h"

MemoryTracker mem_tracker = {0, 0};

// 重写 malloc 和 free 以跟踪内存使用
void* tracked_malloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr) {
        mem_tracker.total_allocated += size;
    }
    return ptr;
}

void tracked_free(void* ptr, size_t size) {
    free(ptr);
    mem_tracker.total_freed += size;
}

// 测试函数
void test_function() {
    // 分配一些内存
    int* arr = (int*)tracked_malloc(10 * sizeof(int));
    assert(arr != NULL);
    tracked_free(arr, 10 * sizeof(int)); // 记得释放内存
}

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

void parse_json(const char *filename) {
    // 读取 JSON 文件
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Could not open file %s\n", filename);
        return;
    }

    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // 读取文件内容
    char *buffer = (char *)malloc(filesize + 1);
    fread(buffer, 1, filesize, file);
    buffer[filesize] = '\0'; // 确保字符串以 null 结尾
    fclose(file);

    // 解析 JSON
    cJSON *json = cJSON_Parse(buffer);
    if (!json) {
        fprintf(stderr, "Error parsing JSON: %s\n", cJSON_GetErrorPtr());
        free(buffer);
        return;
    }

    // 提取数据
    cJSON *name = cJSON_GetObjectItem(json, "name");
    cJSON *age = cJSON_GetObjectItem(json, "age");
    cJSON *is_student = cJSON_GetObjectItem(json, "is_student");
    cJSON *courses = cJSON_GetObjectItem(json, "courses");
    cJSON *address = cJSON_GetObjectItem(json, "address");

    // 打印提取的数据
    printf("Name: %s\n", name->valuestring);
    printf("Age: %d\n", age->valueint);
    printf("Is Student: %s\n", is_student->type == cJSON_True ? "true" : "false");

    // 打印课程
    printf("Courses: ");
    for (int i = 0; i < cJSON_GetArraySize(courses); i++) {
        cJSON *course = cJSON_GetArrayItem(courses, i);
        printf("%s ", course->valuestring);
    }
    printf("\n");

    // 打印地址
    printf("Address:\n");
    printf("  Street: %s\n", cJSON_GetObjectItem(address, "street")->valuestring);
    printf("  City: %s\n", cJSON_GetObjectItem(address, "city")->valuestring);
    printf("  Zip: %s\n", cJSON_GetObjectItem(address, "zip")->valuestring);

    // 释放内存
    cJSON_Delete(json);
    free(buffer);
}




// 更新 JSON 文件中所有 Person 对象的函数
void update_persons(const char *filename) {
    // 打开 JSON 文件
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Could not open file %s\n", filename);
        return;
    }

    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // 读取文件内容到缓冲区
    char *buffer = (char *)malloc(filesize + 1);
    fread(buffer, 1, filesize, file);
    buffer[filesize] = '\0'; // 添加字符串结束符
    fclose(file);

    // 解析 JSON
    cJSON *json = cJSON_Parse(buffer);
    if (!json) {
        fprintf(stderr, "Error parsing JSON: %s\n", cJSON_GetErrorPtr());
        free(buffer);
        return;
    }

    // 遍历 JSON 数组并更新每个对象
    int array_size = cJSON_GetArraySize(json);
    for (int i = 0; i < array_size; i++) {
        cJSON *person = cJSON_GetArrayItem(json, i);
        if (person) {
            // 更新字段
            cJSON_ReplaceItemInObject(person, "name", cJSON_CreateString("Updated Name"));
            cJSON_ReplaceItemInObject(person, "age", cJSON_CreateNumber(25));
            cJSON_ReplaceItemInObject(person, "is_student", cJSON_CreateBool(0));

            // 更新课程
            cJSON *courses = cJSON_GetObjectItem(person, "courses");
            cJSON_DeleteItemFromArray(courses, 0); // 删除第一个课程
            cJSON_AddItemToArray(courses, cJSON_CreateString("Updated Course A")); // 添加新课程

            // 更新地址
            cJSON *address = cJSON_GetObjectItem(person, "address");
            cJSON_ReplaceItemInObject(address, "street", cJSON_CreateString("Updated Street"));
            cJSON_ReplaceItemInObject(address, "city", cJSON_CreateString("Updated City"));
            cJSON_ReplaceItemInObject(address, "zip", cJSON_CreateString("Updated Zip"));
        }
    }

    // 将更新后的 JSON 写回文件
    char *json_string = cJSON_Print(json);
    if (!json_string) {
        fprintf(stderr, "Failed to print JSON object\n");
        cJSON_Delete(json);
        free(buffer);
        return;
    }

    file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Could not open file %s for writing\n", filename);
        free(json_string);
        cJSON_Delete(json);
        free(buffer);
        return;
    }

    fprintf(file, "%s\n", json_string);
    fclose(file);

    // 释放内存
    free(json_string);
    cJSON_Delete(json);
    free(buffer);
}




/****************************************************/

// 创建一个新的元组，指定元组的大小（即包含的元素数量）
Tuple* tuple_create(size_t size) {
    Tuple* tuple = (Tuple*)malloc(sizeof(Tuple));
    if (!tuple) {
        fprintf(stderr, "Failed to allocate memory for tuple.\n");
        exit(EXIT_FAILURE);
    }
    tuple->size = size;
    tuple->elements = (TupleElement*)malloc(sizeof(TupleElement) * size);
    if (!tuple->elements) {
        fprintf(stderr, "Failed to allocate memory for tuple elements.\n");
        free(tuple);
        exit(EXIT_FAILURE);
    }
    // 初始化所有元素类型为无效类型（可根据需要定义）
    for (size_t i = 0; i < size; i++) {
        tuple->elements[i].type = -1; // -1 表示未初始化
    }
    return tuple;
}

// 设置元组中指定索引的元素
void tuple_set_element(Tuple* tuple, size_t index, TupleElement element) {
    if (index >= tuple->size) {
        fprintf(stderr, "Index out of bounds in tuple_set_element.\n");
        return;
    }
    // 如果之前是字符串，释放旧内存
    if (tuple->elements[index].type == TYPE_STRING && tuple->elements[index].data.string_val != NULL) {
        free(tuple->elements[index].data.string_val);
    }
    tuple->elements[index] = element;
}

// 获取元组中指定索引的元素
TupleElement tuple_get_element(const Tuple* tuple, size_t index) {
    if (index >= tuple->size) {
        fprintf(stderr, "Index out of bounds in tuple_get_element.\n");
        TupleElement empty;
        empty.type = -1;
        return empty;
    }
    return tuple->elements[index];
}

// 打印元组中的所有元素
void tuple_print(const Tuple* tuple) {
    printf("(");
    for (size_t i = 0; i < tuple->size; i++) {
        TupleElement elem = tuple->elements[i];
        switch (elem.type) {
            case TYPE_INT:
                printf("%d", elem.data.int_val);
                break;
            case TYPE_FLOAT:
                printf("%f", elem.data.float_val);
                break;
            case TYPE_STRING:
                printf("\"%s\"", elem.data.string_val);
                break;
            default:
                printf("Unknown");
        }
        if (i != tuple->size - 1) {
            printf(", ");
        }
    }
    printf(")\n");
}

// 释放元组占用的内存
void tuple_free(Tuple* tuple) {
    if (!tuple) return;
    // 释放字符串类型的元素
    for (size_t i = 0; i < tuple->size; i++) {
        if (tuple->elements[i].type == TYPE_STRING && tuple->elements[i].data.string_val != NULL) {
            free(tuple->elements[i].data.string_val);
        }
    }
    free(tuple->elements);
    free(tuple);
}


void test_Tuple()
{
     // 创建一个包含3个元素的元组
    Tuple* my_tuple = tuple_create(3);

    // 设置第一个元素为整数
    TupleElement elem1;
    elem1.type = TYPE_INT;
    elem1.data.int_val = 42;
    tuple_set_element(my_tuple, 0, elem1);

    // 设置第二个元素为浮点数
    TupleElement elem2;
    elem2.type = TYPE_FLOAT;
    elem2.data.float_val = 3.1415f;
    tuple_set_element(my_tuple, 1, elem2);

    // 设置第三个元素为字符串
    TupleElement elem3;
    elem3.type = TYPE_STRING;
    elem3.data.string_val = strdup("Hello, Tuple!");
    tuple_set_element(my_tuple, 2, elem3);

    // 打印元组
    printf("My Tuple: ");
    tuple_print(my_tuple); // 输出: (42, 3.141500, "Hello, Tuple!")

    // 获取并打印第三个元素
    TupleElement retrieved = tuple_get_element(my_tuple, 2);
    if (retrieved.type == TYPE_STRING) {
        printf("Third element: \"%s\"\n", retrieved.data.string_val);
    }

    // 释放元组内存
    tuple_free(my_tuple);



}



int  test_paer(void)
{
           // 压力测试
    List* list = create_list();
    if (!list) {
        fprintf(stderr, "Failed to create list.\n");
        return 1;
    }

    // 插入大量数据
    for (int i = 0; i < 100000; i++) {
        int* value = (int*)tracked_malloc(sizeof(int));
        *value = i;
        list_append(list, value);
    }

    printf("Inserted 100000 elements into the list.\n");

    // 删除一些数据
    for (int i = 0; i < 50000; i++) {
        Node* node = list_node_first(list);
        if (node) {
            int* value = (int*)list_remove(list, node);
            tracked_free(value, sizeof(int)); // 释放内存
        }
    }

    printf("Deleted 50000 elements from the list.\n");

    // 打印内存使用情况
    printf("Total allocated memory: %zu bytes\n", mem_tracker.total_allocated);
    printf("Total freed memory: %zu bytes\n", mem_tracker.total_freed);
    printf("Current memory usage: %zu bytes\n", mem_tracker.total_allocated - mem_tracker.total_freed);

    // 释放链表
    free_list(list);


    const char *filename = "data.json"; // JSON 文件名
    parse_json(filename);


      // 创建一个 JSON 对象
    cJSON *root = cJSON_CreateObject();
    
    // 添加字符串到 JSON 对象
    cJSON_AddStringToObject(root, "name", "John Doe");
    cJSON_AddStringToObject(root, "city", "Anytown");
    
    // 打印 JSON 对象
    char *json_string = cJSON_Print(root);
    printf("%s\n", json_string);

    // 清理
    free(json_string);
    cJSON_Delete(root);
}


void move_elements(List *l, List *lb, size_t i, size_t len){
    Node *current = l->head;
    Node *to_move = NULL;

    //找到要移动的节点  
    for(size_t j = 0;j<len && current != NULL;j++){
        if(j < i ){
            //记录这个要移动的节点
            to_move = current;
            current = current->next;
        }else{
            //移除这个节点
            list_remove(l,current);
            //插入到lb中
            list_insert_sorted(lb, to_move->data); // 插入到 lb 中
            current = current->next;
        }   
    }



}
