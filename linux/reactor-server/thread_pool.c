// thread_pool.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "thread_pool.h"
#include "global_queue.h"

static void* worker_func(void* arg) {
    (void)arg; // 不使用参数
    
    printf("[Worker] Thread started\n");

    while (1) {
        global_task_t task;
        
        // 尝试从全局队列获取任务
        if (global_queue_get_task(&task) == 0) {
            printf("[Worker] Received from fd=%d: %s", task.fd, task.data);

            // 生成 echo 响应数据
            char response[MAX_DATA_SIZE + 100];
            snprintf(response, sizeof(response), "Echo: %s", task.data);

            // TODO: 这里应该将响应发送回客户端
            // 为了简化，暂时只打印处理完成的消息
            printf("[Worker] Generated response for fd=%d: %s", task.fd, response);
            
        } else {
            // 没有任务，休眠一下
            usleep(10000); // 10ms
        }
    }

    return NULL;
}

void thread_pool_init(thread_pool_t* pool, int thread_count) {
    pool->thread_count = thread_count;
    pool->threads = malloc(sizeof(pthread_t) * thread_count);

    for (int i = 0; i < thread_count; ++i) {
        int ret = pthread_create(&pool->threads[i], NULL, worker_func, NULL);
        if (ret != 0) {
            printf("[ThreadPool] pthread_create failed: %d\n", ret);
        }
    }
    
    printf("[ThreadPool] Created %d worker threads\n", thread_count);
}

void thread_pool_destroy(thread_pool_t* pool) {
    // 不做线程退出处理，直接释放内存
    free(pool->threads);
}