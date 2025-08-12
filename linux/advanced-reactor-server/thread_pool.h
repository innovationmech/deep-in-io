// thread_pool.h
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "common.h"
#include "task_queue.h"

typedef struct thread_pool {
    pthread_t *threads;
    int thread_count;
    task_queue_t *task_queue;
    int shutdown;
    
    // 统计信息
    long tasks_completed;
    pthread_mutex_t stats_mutex;
} thread_pool_t;

// 创建线程池
thread_pool_t* thread_pool_create(int thread_count, int queue_size);

// 销毁线程池
void thread_pool_destroy(thread_pool_t *pool);

// 提交任务到线程池
int thread_pool_submit(thread_pool_t *pool, task_t *task);

// 关闭线程池
void thread_pool_shutdown(thread_pool_t *pool);

#endif // THREAD_POOL_H