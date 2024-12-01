#ifndef MEMPOOL_H
#define MEMPOOL_H

#include <stddef.h>

// 内存池结构
typedef struct {
    size_t block_size;  // 每个块的大小
    size_t block_count; // 块的数量
    void* memory;       // 内存池的起始地址
    void** free_list;   // 空闲块链表
    size_t free_count;  // 空闲块的数量
} MemoryPool;

// 创建内存池
MemoryPool* mempool_create(size_t block_size, size_t block_count);

// 从内存池分配内存
void* mempool_alloc(MemoryPool* pool);

// 释放内存回内存池
void mempool_free(MemoryPool* pool, void* ptr);

// 销毁内存池
void mempool_destroy(MemoryPool* pool);

#endif // MEMPOOL_H


