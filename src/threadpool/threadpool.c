#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "threadpool.h"
#include "tr_text.h"

// 线程池工作函数
static void* threadpool_thread(void* threadpool);

// 创建线程池
ThreadPool* threadpool_create(int thread_count, int queue_size) {
    ThreadPool* pool;
    int i;

    if ((pool = (ThreadPool*)malloc(sizeof(ThreadPool))) == NULL) {
        goto err;
    }

    // 初始化
    pool->thread_count = 0;
    pool->queue_size = queue_size;
    pool->head = pool->tail = pool->count = 0;
    pool->shutdown = pool->started = 0;

    // 分配线程和任务队列
    pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * thread_count);
    pool->task_queue = (ThreadPoolTask*)malloc(sizeof(ThreadPoolTask) * queue_size);

    if ((pthread_mutex_init(&(pool->lock), NULL) != 0) ||
        (pthread_cond_init(&(pool->notify), NULL) != 0) ||
        (pool->threads == NULL) ||
        (pool->task_queue == NULL)) {
        goto err;
    }

    // 启动线程
    for (i = 0; i < thread_count; i++) {
        if (pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void*)pool) != 0) {
            threadpool_destroy(pool);
            return NULL;
        }
        pool->thread_count++;
        pool->started++;
    }

    return pool;

err:
    if (pool) {
        threadpool_destroy(pool);
    }
    return NULL;
}

// 添加任务到线程池
int threadpool_add(ThreadPool* pool, void (*function)(void*), void* argument) {
    int next, err = 0;

    if (pool == NULL || function == NULL) {
        return -1;
    }

    if (pthread_mutex_lock(&(pool->lock)) != 0) {
        return -1;
    }

    next = (pool->tail + 1) % pool->queue_size;

    do {
        // 队列已满
        if (pool->count == pool->queue_size) {
            err = -1;
            break;
        }

        // 添加任务到队列
        pool->task_queue[pool->tail].function = function;
        pool->task_queue[pool->tail].argument = argument;
        pool->tail = next;
        pool->count += 1;

        // 通知线程
        if (pthread_cond_signal(&(pool->notify)) != 0) {
            err = -1;
            break;
        }
    } while (0);

    if (pthread_mutex_unlock(&pool->lock) != 0) {
        err = -1;
    }

    return err;
}

// 销毁线程池
int threadpool_destroy(ThreadPool* pool) {
    int i, err = 0;

    if (pool == NULL) {
        return -1;
    }

    if (pthread_mutex_lock(&(pool->lock)) != 0) {
        return -1;
    }

    do {
        // 设置关闭标志
        pool->shutdown = 1;

        // 通知所有线程
        if ((pthread_cond_broadcast(&(pool->notify)) != 0) ||
            (pthread_mutex_unlock(&(pool->lock)) != 0)) {
            err = -1;
            break;
        }

        // 等待所有线程结束
        for (i = 0; i < pool->thread_count; i++) {
            if (pthread_join(pool->threads[i], NULL) != 0) {
                err = -1;
            }
        }
    } while (0);

    if (!err) {
        threadpool_free(pool);
    }
    return err;
}

// 释放线程池
int threadpool_free(ThreadPool* pool) {
    if (pool == NULL || pool->started > 0) {
        return -1;
    }

    if (pool->threads) {
        free(pool->threads);
        free(pool->task_queue);

        pthread_mutex_lock(&(pool->lock));
        pthread_mutex_destroy(&(pool->lock));
        pthread_cond_destroy(&(pool->notify));
    }
    free(pool);
    return 0;
}

// 线程池工作函数
static void* threadpool_thread(void* threadpool) {
    ThreadPool* pool = (ThreadPool*)threadpool;
    ThreadPoolTask task;

    for (;;) {
        pthread_mutex_lock(&(pool->lock));

        // 等待任务
        while ((pool->count == 0) && (!pool->shutdown)) {
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }

        if ((pool->shutdown)) {
            break;
        }

        // 获取任务
        task.function = pool->task_queue[pool->head].function;
        task.argument = pool->task_queue[pool->head].argument;
        pool->head = (pool->head + 1) % pool->queue_size;
        pool->count -= 1;

        pthread_mutex_unlock(&(pool->lock));

        // 执行任务
        (*(task.function))(task.argument);
    }

    pool->started--;

    pthread_mutex_unlock(&(pool->lock));
    pthread_exit(NULL);
return (NULL);
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


