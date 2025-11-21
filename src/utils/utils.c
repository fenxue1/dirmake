/*
 * @Author: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @Date: 2024-11-17 22:59:06
 * @LastEditors: fenxue1 99110925+fenxue1@users.noreply.github.com
 * @LastEditTime: 2025-10-08 22:19:13
 * @FilePath: \test_cmake\src\utils\utils.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "utils.h"
#include "tr_text.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
int add(int a, int b) {
    return a + b;
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






// 定义事件类型
typedef enum {
    EVENT_BUTTON_CLICK,
    EVENT_MOUSE_MOVE,
    EVENT_KEY_PRESS,
    EVENT_CUSTOM_1,
    EVENT_CUSTOM_2,
    // 可以添加更多事件类型
    EVENT_COUNT
} EventType;

// 定义事件结构体
typedef struct {
    EventType type;
    void* data;
} Event;

// 定义事件处理器函数指针
typedef void (*EventHandler)(Event* event);

// 定义链表节点，用于存储事件处理器
typedef struct EventHandlerNode {
    EventHandler handler;
    struct EventHandlerNode* next;
} EventHandlerNode;

// 定义对象结构体，包含事件处理器链表
typedef struct {
    EventHandlerNode* handlers[EVENT_COUNT];
} Object;

// 事件处理器函数示例：处理按钮点击事件
void handle_button_click(Event* event) {
    printf("Button clicked!\n");
}

// 事件处理器函数示例：处理鼠标移动事件
void handle_mouse_move(Event* event) {
    printf("Mouse moved!\n");
}

// 事件处理器函数示例：处理按键按下事件
void handle_key_press(Event* event) {
    printf("Key pressed!\n");
}

// 初始化对象，设置事件处理器
void init_object(Object* obj) {
    for (int i = 0; i < EVENT_COUNT; ++i) {
        obj->handlers[i] = NULL;
    }
}

// 注册事件处理器
void register_event_handler(Object* obj, EventType type, EventHandler handler) {
    EventHandlerNode* node = (EventHandlerNode*)malloc(sizeof(EventHandlerNode));
    node->handler = handler;
    node->next = obj->handlers[type];
    obj->handlers[type] = node;
}

// 注销事件处理器
void unregister_event_handler(Object* obj, EventType type, EventHandler handler) {
    EventHandlerNode* curr = obj->handlers[type];
    EventHandlerNode* prev = NULL;
    while (curr) {
        if (curr->handler == handler) {
            if (prev) {
                prev->next = curr->next;
            } else {
                obj->handlers[type] = curr->next;
            }
            free(curr);
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

// 触发事件
void trigger_event(Object* obj, Event* event) {
    EventHandlerNode* curr = obj->handlers[event->type];
    while (curr) {
        curr->handler(event);
        curr = curr->next;
    }
}

// 事件队列节点
typedef struct EventQueueNode {
    Event event;
    struct EventQueueNode* next;
} EventQueueNode;

// 事件队列
typedef struct {
    EventQueueNode* head;
    EventQueueNode* tail;
} EventQueue;

// 初始化事件队列
void init_event_queue(EventQueue* queue) {
    queue->head = NULL;
    queue->tail = NULL;
}

// 入队事件
void enqueue_event(EventQueue* queue, Event event) {
    EventQueueNode* node = (EventQueueNode*)malloc(sizeof(EventQueueNode));
    node->event = event;
    node->next = NULL;
    if (queue->tail) {
        queue->tail->next = node;
    } else {
        queue->head = node;
    }
    queue->tail = node;
}

// 出队事件
bool dequeue_event(EventQueue* queue, Event* event) {
    if (!queue->head) {
        return false;
    }
    *event = queue->head->event;
    EventQueueNode* temp = queue->head;
    queue->head = queue->head->next;
    if (!queue->head) {
        queue->tail = NULL;
    }
    free(temp);
    return true;
}

// 处理事件队列中的事件
void process_event_queue(Object* obj, EventQueue* queue) {
    Event event;
    while (dequeue_event(queue, &event)) {
        trigger_event(obj, &event);
    }
}

// 事件过滤器函数指针
typedef bool (*EventFilter)(Event* event);

// 示例事件过滤器
bool filter_button_click(Event* event) {
    if (event->type == EVENT_BUTTON_CLICK) {
        // 仅允许偶数次点击通过
        static int click_count = 0;
        click_count++;
        return (click_count % 2 == 0);
    }
    return true;
}

// 触发事件并应用过滤器
void trigger_event_with_filter(Object* obj, Event* event, EventFilter filter) {
    if (filter(event)) {
        trigger_event(obj, event);
    }
}

// int main() {
//     Object obj;
//     init_object(&obj);
//     register_event_handler(&obj, EVENT_BUTTON_CLICK, handle_button_click);
//     register_event_handler(&obj, EVENT_MOUSE_MOVE, handle_mouse_move);
//     register_event_handler(&obj, EVENT_KEY_PRESS, handle_key_press);

//     EventQueue queue;
//     init_event_queue(&queue);

//     Event button_click_event = { EVENT_BUTTON_CLICK, NULL };
//     Event mouse_move_event = { EVENT_MOUSE_MOVE, NULL };
//     Event key_press_event = { EVENT_KEY_PRESS, NULL };

//     enqueue_event(&queue, button_click_event);
//     enqueue_event(&queue, mouse_move_event);
//     enqueue_event(&queue, key_press_event);

//     EventFilter filter = filter_button_click;
//     Event event;
//     while (dequeue_event(&queue, &event)) {
//         trigger_event_with_filter(&obj, &event, filter);
//     }

//     return 0;
// }


// 创建新的孩子节点
CTNode* createChildNode(int childIndex) {
    CTNode* newNode = (CTNode*)malloc(sizeof(CTNode));
    newNode->child = childIndex;
    newNode->next = NULL;
    return newNode;
}

// 添加孩子节点
void addChild(CTree* tree, int parentIndex, int childIndex) {
    if (parentIndex < 0 || parentIndex >= tree->n || childIndex < 0 || childIndex >= tree->n) {
        printf("无效的索引\n");
        return;
    }

    CTNode* newChild = createChildNode(childIndex);
    if (tree->nodes[parentIndex].firstchild == NULL) {
        tree->nodes[parentIndex].firstchild = newChild; // 如果没有孩子，直接添加
    } else {
        CTNode* temp = tree->nodes[parentIndex].firstchild;
        while (temp->next != NULL) {
            temp = temp->next; // 找到最后一个孩子
        }
        temp->next = newChild; // 添加到最后
    }
}

// 计算节点的度
int calculateDegree(CTree* tree, int index) {
    if (index < 0 || index >= tree->n) {
        return -1; // 无效的索引
    }

    CTNode* child = tree->nodes[index].firstchild;
    int degree = 0;

    // 遍历所有孩子节点
    while (child != NULL) {
        degree++;
        child = child->next; // 移动到下一个兄弟节点
    }

    return degree;
}

// 打印树的结构和每个节点的度
void printTreeAndDegrees(CTree* tree) {
    for (int i = 0; i < tree->n; i++) {
        printf("节点 %d (数据: %d): ", i, tree->nodes[i].data);
        CTNode* child = tree->nodes[i].firstchild;
        while (child != NULL) {
            printf("%d ", child->child);
            child = child->next;
        }
        int degree = calculateDegree(tree, i);
        printf(" | 度: %d\n", degree);
    }
}




