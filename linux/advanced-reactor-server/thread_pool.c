// thread_pool.c
#include "thread_pool.h"

// 业务处理函数（示例：简单的 HTTP echo 服务）
static void process_request(connection_t *conn, void *data, int data_len) {
    // 这里模拟业务处理，实际可以解析 HTTP 请求、查询数据库等
    char response[BUFFER_SIZE];
    int response_len;
    
    // 简单的 HTTP 响应
    const char *http_response_template = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %d\r\n"
        "Connection: keep-alive\r\n"
        "\r\n"
        "%s";
    
    // Echo 收到的数据
    char body[1024];
    snprintf(body, sizeof(body), "Echo: %.*s", data_len, (char*)data);
    
    response_len = snprintf(response, sizeof(response), 
                           http_response_template, 
                           (int)strlen(body), body);
    
    // 将响应数据写入连接的写缓冲区
    if (response_len < BUFFER_SIZE) {
        memcpy(conn->write_buf, response, response_len);
        conn->write_size = response_len;
        conn->write_pos = 0;
        conn->state = CONN_STATE_WRITING;
    }
    
    // 通知 IO 线程可以写入数据
    struct epoll_event ev;
    ev.events = EPOLLOUT | EPOLLET;
    ev.data.ptr = conn;
    if (epoll_ctl(conn->epoll_fd, EPOLL_CTL_MOD, conn->fd, &ev) == -1) {
        log_error("Failed to modify epoll event for fd=%d: %s", conn->fd, strerror(errno));
    } else {
        log_info("Response prepared for fd=%d, switching to EPOLLOUT", conn->fd);
    }
}

// 工作线程函数
static void* worker_thread(void *arg) {
    thread_pool_t *pool = (thread_pool_t*)arg;
    
    while (!pool->shutdown) {
        task_t *task = task_queue_pop(pool->task_queue);
        if (!task) {
            if (pool->shutdown) break;
            continue;
        }
        
        // 根据任务类型处理
        switch (task->type) {
            case TASK_TYPE_PROCESS:
                // 处理业务逻辑
                log_info("Processing request for fd=%d, data_len=%d", task->conn->fd, task->data_len);
                process_request(task->conn, task->data, task->data_len);
                break;
                
            case TASK_TYPE_CLOSE:
                // 清理连接
                if (task->conn) {
                    close(task->conn->fd);
                    free(task->conn);
                }
                break;
                
            default:
                log_error("Unknown task type: %d", task->type);
                break;
        }
        
        // 更新统计信息
        pthread_mutex_lock(&pool->stats_mutex);
        pool->tasks_completed++;
        pthread_mutex_unlock(&pool->stats_mutex);
        
        // 销毁任务
        task_destroy(task);
    }
    
    return NULL;
}

thread_pool_t* thread_pool_create(int thread_count, int queue_size) {
    thread_pool_t *pool = (thread_pool_t*)malloc(sizeof(thread_pool_t));
    if (!pool) return NULL;
    
    pool->thread_count = thread_count;
    pool->shutdown = 0;
    pool->tasks_completed = 0;
    
    // 创建任务队列
    pool->task_queue = task_queue_create(queue_size);
    if (!pool->task_queue) {
        free(pool);
        return NULL;
    }
    
    // 初始化统计互斥锁
    pthread_mutex_init(&pool->stats_mutex, NULL);
    
    // 创建工作线程
    pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * thread_count);
    if (!pool->threads) {
        task_queue_destroy(pool->task_queue);
        free(pool);
        return NULL;
    }
    
    for (int i = 0; i < thread_count; i++) {
        if (pthread_create(&pool->threads[i], NULL, worker_thread, pool) != 0) {
            log_error("Failed to create worker thread %d", i);
            // 清理已创建的线程
            pool->thread_count = i;
            thread_pool_destroy(pool);
            return NULL;
        }
    }
    
    log_info("Thread pool created with %d workers", thread_count);
    
    return pool;
}

void thread_pool_destroy(thread_pool_t *pool) {
    if (!pool) return;
    
    // 设置关闭标志
    pool->shutdown = 1;
    
    // 关闭任务队列
    task_queue_shutdown(pool->task_queue);
    
    // 等待所有工作线程退出
    for (int i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }
    
    // 释放资源
    free(pool->threads);
    task_queue_destroy(pool->task_queue);
    pthread_mutex_destroy(&pool->stats_mutex);
    
    log_info("Thread pool destroyed. Total tasks completed: %ld", pool->tasks_completed);
    
    free(pool);
}

int thread_pool_submit(thread_pool_t *pool, task_t *task) {
    if (!pool || !task || pool->shutdown) return -1;
    
    return task_queue_push(pool->task_queue, task);
}

void thread_pool_shutdown(thread_pool_t *pool) {
    if (!pool) return;
    pool->shutdown = 1;
    task_queue_shutdown(pool->task_queue);
}