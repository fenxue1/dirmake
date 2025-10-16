#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mempool.h"
#include "tr_text.h"

// 创建内存池
MemoryPool* mempool_create(size_t block_size, size_t block_count) {
    MemoryPool* pool = (MemoryPool*)malloc(sizeof(MemoryPool));
    if (!pool) return NULL;

    pool->block_size = block_size;
    pool->block_count = block_count;
    pool->memory = malloc(block_size * block_count);
    pool->free_list = (void**)malloc(sizeof(void*) * block_count);
    pool->free_count = block_count;

    if (!pool->memory || !pool->free_list) {
        free(pool->memory);
        free(pool->free_list);
        free(pool);
        return NULL;
    }

    // 初始化空闲链表
    for (size_t i = 0; i < block_count; i++) {
        pool->free_list[i] = (char*)pool->memory + i * block_size;
    }

    return pool;
}

// 从内存池分配内存
void* mempool_alloc(MemoryPool* pool) {
    if (pool->free_count == 0) {
        return NULL; // 内存池已满
    }

    // 从空闲链表中取出一个块
    void* block = pool->free_list[--pool->free_count];
    return block;
}

// 释放内存回内存池
void mempool_free(MemoryPool* pool, void* ptr) {
    if (pool->free_count < pool->block_count) {
        // 将块放回空闲链表
        pool->free_list[pool->free_count++] = ptr;
    }
}

// 销毁内存池
void mempool_destroy(MemoryPool* pool) {
    if (pool) {
        free(pool->memory);
        free(pool->free_list);
        free(pool);
    }
}

static const _Tr_TEXT txt_input_points = {
    "输入点",
    "Input Points",
    "Điểm nhập vào",
    "입력 포인트",
    "Giriş Noktaları",
    "\x4e\xc3\xba\x6d\x65\x72\x6f\x20\x64\x65\x20\x63\xc3\xb3\x64\x69\x67\x6f\x73\x3a",
    "Puntos de entrada",
    "Pontos de entrada",
    "نقاط ورودی",
    "入力ポイント",
    "نقاط الإدخال",
    "其它"
};