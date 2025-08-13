// server.h
#ifndef SERVER_H
#define SERVER_H

#include "common.h"
#include "thread_pool.h"
#include "io_thread.h"
#include "event_loop.h"

typedef struct reactor_server {
    int listen_fd;
    int port;
    
    // 线程池
    thread_pool_t *worker_pool;
    io_thread_pool_t *io_pool;
    
    // 主线程 event loop（只监听 listen_fd）
    event_loop_t *main_event_loop;
    
    // 运行状态
    volatile int running;
    
    // 统计信息
    long total_connections;
    pthread_mutex_t stats_mutex;
} reactor_server_t;

// 创建服务器
reactor_server_t* server_create(int port, int io_threads, int worker_threads);

// 启动服务器
int server_start(reactor_server_t *server);

// 停止服务器
void server_stop(reactor_server_t *server);

// 销毁服务器
void server_destroy(reactor_server_t *server);

#endif // SERVER_H