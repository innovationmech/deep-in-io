// global_queue.h - 简化的全局任务处理
#ifndef GLOBAL_QUEUE_H
#define GLOBAL_QUEUE_H

#include <pthread.h>

#define MAX_TASKS 1024
#define MAX_DATA_SIZE 4096

typedef struct {
    int fd;
    char data[MAX_DATA_SIZE];
    int valid;
} global_task_t;

// 全局任务数组和锁
extern global_task_t g_tasks[MAX_TASKS];
extern pthread_mutex_t g_task_lock;
extern int g_task_write_index;

void global_queue_init();
int global_queue_add_task(int fd, const char* data);
int global_queue_get_task(global_task_t* task);

#endif // GLOBAL_QUEUE_H