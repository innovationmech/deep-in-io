// global_queue.c
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "global_queue.h"

// 全局变量定义
global_task_t g_tasks[MAX_TASKS];
pthread_mutex_t g_task_lock = PTHREAD_MUTEX_INITIALIZER;
int g_task_write_index = 0;

void global_queue_init() {
    for (int i = 0; i < MAX_TASKS; i++) {
        g_tasks[i].valid = 0;
        g_tasks[i].fd = -1;
        g_tasks[i].data[0] = '\0';
    }
    printf("[GlobalQueue] Initialized\n");
}

int global_queue_add_task(int fd, const char* data) {
    pthread_mutex_lock(&g_task_lock);
    
    // 找一个空闲位置
    for (int i = 0; i < MAX_TASKS; i++) {
        if (!g_tasks[i].valid) {
            g_tasks[i].fd = fd;
            strncpy(g_tasks[i].data, data, MAX_DATA_SIZE - 1);
            g_tasks[i].data[MAX_DATA_SIZE - 1] = '\0';
            g_tasks[i].valid = 1;
            
            printf("[GlobalQueue] Added task: fd=%d, data='%s'\n", fd, data);
            pthread_mutex_unlock(&g_task_lock);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&g_task_lock);
    printf("[GlobalQueue] Queue full!\n");
    return -1;
}

int global_queue_get_task(global_task_t* task) {
    pthread_mutex_lock(&g_task_lock);
    
    // 找一个有效任务
    for (int i = 0; i < MAX_TASKS; i++) {
        if (g_tasks[i].valid) {
            *task = g_tasks[i];
            g_tasks[i].valid = 0; // 标记为已取出
            
            printf("[GlobalQueue] Got task: fd=%d, data='%s'\n", task->fd, task->data);
            pthread_mutex_unlock(&g_task_lock);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&g_task_lock);
    return -1; // 没有任务
}