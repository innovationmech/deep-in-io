// task_queue.h
#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include "common.h"

typedef struct task_queue {
    task_t *head;
    task_t *tail;
    int size;
    int max_size;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    int shutdown;
} task_queue_t;

// 创建任务队列
task_queue_t* task_queue_create(int max_size);

// 销毁任务队列
void task_queue_destroy(task_queue_t *queue);

// 添加任务（生产者）
int task_queue_push(task_queue_t *queue, task_t *task);

// 获取任务（消费者）
task_t* task_queue_pop(task_queue_t *queue);

// 创建任务
task_t* task_create(task_type_t type, connection_t *conn, void *data, int data_len);

// 销毁任务
void task_destroy(task_t *task);

// 设置队列为关闭状态
void task_queue_shutdown(task_queue_t *queue);

#endif // TASK_QUEUE_H
