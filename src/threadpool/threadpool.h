#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>

// 任务结构
typedef struct {
    void (*function)(void*); // 任务函数
    void* argument;          // 任务参数
} ThreadPoolTask;

// 线程池结构
typedef struct {
    pthread_mutex_t lock;          // 互斥锁
    pthread_cond_t notify;         // 条件变量
    pthread_t* threads;            // 线程数组
    ThreadPoolTask* task_queue;    // 任务队列
    int thread_count;              // 线程数
    int queue_size;                // 队列大小
    int head;                      // 队列头
    int tail;                      // 队列尾
    int count;                     // 当前任务数
    int shutdown;                  // 关闭标志
    int started;                   // 已启动的线程数
} ThreadPool;

// 创建线程池
ThreadPool* threadpool_create(int thread_count, int queue_size);

// 添加任务到线程池
int threadpool_add(ThreadPool* pool, void (*function)(void*), void* argument);

// 销毁线程池
int threadpool_destroy(ThreadPool* pool);
int threadpool_free(ThreadPool* pool);
#endif // THREADPOOL_H