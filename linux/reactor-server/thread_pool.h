// thread_pool.h
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>

typedef struct thread_pool_t {
    int thread_count;
    pthread_t* threads;
} thread_pool_t;

void thread_pool_init(thread_pool_t* pool, int thread_count);
void thread_pool_destroy(thread_pool_t* pool);

#endif // THREAD_POOL_H