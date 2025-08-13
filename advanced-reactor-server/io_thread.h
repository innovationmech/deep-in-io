// io_thread.h
#ifndef IO_THREAD_H
#define IO_THREAD_H

#include "common.h"
#include "event_loop.h"
#include "thread_pool.h"

typedef struct io_thread {
    pthread_t thread_id;
    int thread_index;
    event_loop_t *event_loop;  // Changed from epoll_wrapper_t
    thread_pool_t *worker_pool;
    int pipe_fd[2];        // 用于主线程通知 IO 线程
    int shutdown;
    
    // 消息队列
    io_message_t *msg_queue_head;
    io_message_t *msg_queue_tail;
    pthread_mutex_t msg_queue_mutex;
    int msg_pipe_fd[2];    // 用于消息通知的管道
    
    // 统计信息
    long connections_handled;
    long bytes_read;
    long bytes_written;
    pthread_mutex_t stats_mutex;
} io_thread_t;

// IO 线程池
typedef struct io_thread_pool {
    io_thread_t **threads;
    int thread_count;
    int next_thread;       // 轮询分配连接
    pthread_mutex_t mutex;
    thread_pool_t *worker_pool;
} io_thread_pool_t;

// 创建 IO 线程池
io_thread_pool_t* io_thread_pool_create(int io_thread_count, thread_pool_t *worker_pool);

// 销毁 IO 线程池
void io_thread_pool_destroy(io_thread_pool_t *pool);

// 分配连接到 IO 线程（轮询）
io_thread_t* io_thread_pool_get_thread(io_thread_pool_t *pool);

// 添加连接到 IO 线程
int io_thread_add_connection(io_thread_t *io_thread, int client_fd, struct sockaddr_in *addr);

// 向IO线程发送消息
void io_thread_send_message(io_thread_t *io_thread, io_msg_type_t type, connection_t *conn);

#endif // IO_THREAD_H