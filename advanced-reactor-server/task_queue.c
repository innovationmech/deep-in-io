// task_queue.c
#include "task_queue.h"
#include "common.h"

task_queue_t* task_queue_create(int max_size) {
    task_queue_t *queue = (task_queue_t*)malloc(sizeof(task_queue_t));
    if (!queue) return NULL;
    
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    queue->max_size = max_size;
    queue->shutdown = 0;
    
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->not_empty, NULL);
    pthread_cond_init(&queue->not_full, NULL);
    
    return queue;
}

void task_queue_destroy(task_queue_t *queue) {
    if (!queue) return;
    
    pthread_mutex_lock(&queue->mutex);
    
    // 清理所有未处理的任务
    task_t *current = queue->head;
    while (current) {
        task_t *next = current->next;
        task_destroy(current);
        current = next;
    }
    
    pthread_mutex_unlock(&queue->mutex);
    
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);
    
    free(queue);
}

int task_queue_push(task_queue_t *queue, task_t *task) {
    if (!queue || !task) return -1;
    
    pthread_mutex_lock(&queue->mutex);
    
    // 等待队列不满
    while (queue->size >= queue->max_size && !queue->shutdown) {
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }
    
    if (queue->shutdown) {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }
    
    // 添加任务到队尾
    task->next = NULL;
    if (queue->tail) {
        queue->tail->next = task;
        queue->tail = task;
    } else {
        queue->head = queue->tail = task;
    }
    
    queue->size++;
    
    // 通知等待的消费者
    pthread_cond_signal(&queue->not_empty);
    
    pthread_mutex_unlock(&queue->mutex);
    
    return 0;
}

task_t* task_queue_pop(task_queue_t *queue) {
    if (!queue) return NULL;
    
    pthread_mutex_lock(&queue->mutex);
    
    // 等待队列非空
    while (queue->size == 0 && !queue->shutdown) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }
    
    if (queue->shutdown && queue->size == 0) {
        pthread_mutex_unlock(&queue->mutex);
        return NULL;
    }
    
    // 从队首取出任务
    task_t *task = queue->head;
    if (task) {
        queue->head = task->next;
        if (!queue->head) {
            queue->tail = NULL;
        }
        queue->size--;
        task->next = NULL;
        
        // 通知等待的生产者
        pthread_cond_signal(&queue->not_full);
    }
    
    pthread_mutex_unlock(&queue->mutex);
    
    return task;
}

task_t* task_create(task_type_t type, connection_t *conn, void *data, int data_len) {
    task_t *task = (task_t*)malloc(sizeof(task_t));
    if (!task) return NULL;
    
    task->type = type;
    task->conn = conn;
    task->next = NULL;
    
    // 增加连接引用计数
    if (conn) {
        conn_acquire(conn);
    }
    
    if (data && data_len > 0) {
        task->data = malloc(data_len);
        if (task->data) {
            memcpy(task->data, data, data_len);
            task->data_len = data_len;
        } else {
            free(task);
            return NULL;
        }
    } else {
        task->data = NULL;
        task->data_len = 0;
    }
    
    return task;
}

void task_destroy(task_t *task) {
    if (!task) return;
    
    // 释放连接引用
    if (task->conn) {
        conn_release(task->conn);
    }
    
    if (task->data) {
        free(task->data);
    }
    free(task);
}

void task_queue_shutdown(task_queue_t *queue) {
    if (!queue) return;
    
    pthread_mutex_lock(&queue->mutex);
    queue->shutdown = 1;
    pthread_cond_broadcast(&queue->not_empty);
    pthread_cond_broadcast(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);
}