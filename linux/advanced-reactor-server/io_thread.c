// io_thread.c
#include "io_thread.h"

// 处理读事件
static void handle_read(io_thread_t *io_thread, connection_t *conn) {
    char buffer[BUFFER_SIZE];
    int n;
    
    while (1) {
        n = read(conn->fd, buffer, sizeof(buffer));
        
        if (n > 0) {
            // 更新统计
            pthread_mutex_lock(&io_thread->stats_mutex);
            io_thread->bytes_read += n;
            pthread_mutex_unlock(&io_thread->stats_mutex);
            
            // 创建处理任务并提交到工作线程池
            log_info("Read %d bytes from fd=%d", n, conn->fd);
            task_t *task = task_create(TASK_TYPE_PROCESS, conn, buffer, n);
            if (task) {
                log_info("Submitting task for fd=%d to worker pool", conn->fd);
                thread_pool_submit(io_thread->worker_pool, task);
            } else {
                log_error("Failed to create task for fd=%d", conn->fd);
            }
            
            conn->last_active = time(NULL);
        } else if (n == 0) {
            // 连接关闭
            log_info("Connection closed by client: fd=%d", conn->fd);
            epoll_wrapper_del(io_thread->epoll, conn->fd);
            close(conn->fd);
            free(conn);
            return;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 数据读取完毕
                break;
            } else {
                log_error("Read error: %s", strerror(errno));
                epoll_wrapper_del(io_thread->epoll, conn->fd);
                close(conn->fd);
                free(conn);
                return;
            }
        }
    }
}

// 处理写事件
static void handle_write(io_thread_t *io_thread, connection_t *conn) {
    if (conn->write_size <= 0) {
        // 没有数据要写，切换回读模式
        epoll_wrapper_mod(io_thread->epoll, conn->fd, EPOLLIN | EPOLLET, conn);
        conn->state = CONN_STATE_READING;
        return;
    }
    
    int n;
    while (conn->write_pos < conn->write_size) {
        n = write(conn->fd, conn->write_buf + conn->write_pos, 
                 conn->write_size - conn->write_pos);
        
        if (n > 0) {
            conn->write_pos += n;
            
            // 更新统计
            pthread_mutex_lock(&io_thread->stats_mutex);
            io_thread->bytes_written += n;
            pthread_mutex_unlock(&io_thread->stats_mutex);
            
            conn->last_active = time(NULL);
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 暂时无法写入，等待下次事件
                break;
            } else {
                log_error("Write error: %s", strerror(errno));
                epoll_wrapper_del(io_thread->epoll, conn->fd);
                close(conn->fd);
                free(conn);
                return;
            }
        }
    }
    
    // 数据写完，切换回读模式
    if (conn->write_pos >= conn->write_size) {
        conn->write_pos = 0;
        conn->write_size = 0;
        epoll_wrapper_mod(io_thread->epoll, conn->fd, EPOLLIN | EPOLLET, conn);
        conn->state = CONN_STATE_READING;
    }
}

// IO 线程主函数
static void* io_thread_run(void *arg) {
    io_thread_t *io_thread = (io_thread_t*)arg;
    
    log_info("IO thread %d started", io_thread->thread_index);
    
    while (!io_thread->shutdown) {
        int nfds = epoll_wrapper_wait(io_thread->epoll, 1000);
        
        for (int i = 0; i < nfds; i++) {
            struct epoll_event *ev = &io_thread->epoll->events[i];
            
            // 检查是否是管道事件（用于接收新连接）
            if (ev->data.ptr == &io_thread->pipe_fd[0]) {
                // 读取管道数据
                connection_t *new_conn;
                if (read(io_thread->pipe_fd[0], &new_conn, sizeof(new_conn)) == sizeof(new_conn)) {
                    // 将新连接添加到 epoll
                    new_conn->epoll_fd = io_thread->epoll->epfd;
                    new_conn->io_thread = io_thread;
                    
                    if (epoll_wrapper_add(io_thread->epoll, new_conn->fd, 
                                         EPOLLIN | EPOLLET, new_conn) == 0) {
                        pthread_mutex_lock(&io_thread->stats_mutex);
                        io_thread->connections_handled++;
                        pthread_mutex_unlock(&io_thread->stats_mutex);
                        
                        log_info("IO thread %d: new connection fd=%d", 
                                io_thread->thread_index, new_conn->fd);
                    } else {
                        log_error("Failed to add connection to epoll");
                        close(new_conn->fd);
                        free(new_conn);
                    }
                }
                continue;
            }
            
            connection_t *conn = (connection_t*)ev->data.ptr;
            if (!conn) continue;
            
            if (ev->events & EPOLLIN) {
                handle_read(io_thread, conn);
            }
            
            if (ev->events & EPOLLOUT) {
                log_info("EPOLLOUT event for fd=%d", conn->fd);
                handle_write(io_thread, conn);
            }
            
            if (ev->events & (EPOLLERR | EPOLLHUP)) {
                log_info("Connection error/hangup: fd=%d", conn->fd);
                epoll_wrapper_del(io_thread->epoll, conn->fd);
                close(conn->fd);
                free(conn);
            }
        }
    }
    
    log_info("IO thread %d stopped", io_thread->thread_index);
    return NULL;
}

// 创建单个 IO 线程
static io_thread_t* io_thread_create(int index, thread_pool_t *worker_pool) {
    io_thread_t *io_thread = (io_thread_t*)malloc(sizeof(io_thread_t));
    if (!io_thread) return NULL;
    
    io_thread->thread_index = index;
    io_thread->worker_pool = worker_pool;
    io_thread->shutdown = 0;
    io_thread->connections_handled = 0;
    io_thread->bytes_read = 0;
    io_thread->bytes_written = 0;
    
    // 创建管道用于通信
    if (pipe(io_thread->pipe_fd) == -1) {
        free(io_thread);
        return NULL;
    }
    
    // 设置管道为非阻塞
    set_nonblocking(io_thread->pipe_fd[0]);
    set_nonblocking(io_thread->pipe_fd[1]);
    
    pthread_mutex_init(&io_thread->stats_mutex, NULL);
    
    // 创建 epoll 实例
    io_thread->epoll = epoll_wrapper_create(MAX_EVENTS);
    if (!io_thread->epoll) {
        close(io_thread->pipe_fd[0]);
        close(io_thread->pipe_fd[1]);
        free(io_thread);
        return NULL;
    }
    
    // 将管道读端添加到 epoll
    if (epoll_wrapper_add(io_thread->epoll, io_thread->pipe_fd[0], 
                         EPOLLIN | EPOLLET, &io_thread->pipe_fd[0]) == -1) {
        epoll_wrapper_destroy(io_thread->epoll);
        close(io_thread->pipe_fd[0]);
        close(io_thread->pipe_fd[1]);
        free(io_thread);
        return NULL;
    }
    
    // 创建线程
    if (pthread_create(&io_thread->thread_id, NULL, io_thread_run, io_thread) != 0) {
        epoll_wrapper_destroy(io_thread->epoll);
        close(io_thread->pipe_fd[0]);
        close(io_thread->pipe_fd[1]);
        free(io_thread);
        return NULL;
    }
    
    return io_thread;
}

// 销毁 IO 线程
static void io_thread_destroy(io_thread_t *io_thread) {
    if (!io_thread) return;
    
    io_thread->shutdown = 1;
    
    // 通过管道通知线程退出
    char dummy = 1;
    write(io_thread->pipe_fd[1], &dummy, 1);
    
    // 等待线程退出
    pthread_join(io_thread->thread_id, NULL);
    
    // 清理资源
    epoll_wrapper_destroy(io_thread->epoll);
    close(io_thread->pipe_fd[0]);
    close(io_thread->pipe_fd[1]);
    pthread_mutex_destroy(&io_thread->stats_mutex);
    
    log_info("IO thread %d stats: connections=%ld, read=%ld bytes, written=%ld bytes",
            io_thread->thread_index, io_thread->connections_handled,
            io_thread->bytes_read, io_thread->bytes_written);
    
    free(io_thread);
}

// 创建 IO 线程池
io_thread_pool_t* io_thread_pool_create(int io_thread_count, thread_pool_t *worker_pool) {
    io_thread_pool_t *pool = (io_thread_pool_t*)malloc(sizeof(io_thread_pool_t));
    if (!pool) return NULL;
    
    pool->thread_count = io_thread_count;
    pool->next_thread = 0;
    pool->worker_pool = worker_pool;
    pthread_mutex_init(&pool->mutex, NULL);
    
    pool->threads = (io_thread_t**)malloc(sizeof(io_thread_t*) * io_thread_count);
    if (!pool->threads) {
        free(pool);
        return NULL;
    }
    
    for (int i = 0; i < io_thread_count; i++) {
        pool->threads[i] = io_thread_create(i, worker_pool);
        if (!pool->threads[i]) {
            // 清理已创建的线程
            for (int j = 0; j < i; j++) {
                io_thread_destroy(pool->threads[j]);
            }
            free(pool->threads);
            free(pool);
            return NULL;
        }
    }
    
    log_info("IO thread pool created with %d threads", io_thread_count);
    
    return pool;
}

// 销毁 IO 线程池
void io_thread_pool_destroy(io_thread_pool_t *pool) {
    if (!pool) return;
    
    // 销毁所有 IO 线程
    for (int i = 0; i < pool->thread_count; i++) {
        io_thread_destroy(pool->threads[i]);
    }
    
    free(pool->threads);
    pthread_mutex_destroy(&pool->mutex);
    free(pool);
    
    log_info("IO thread pool destroyed");
}

// 轮询获取下一个 IO 线程
io_thread_t* io_thread_pool_get_thread(io_thread_pool_t *pool) {
    if (!pool || pool->thread_count == 0) return NULL;
    
    pthread_mutex_lock(&pool->mutex);
    io_thread_t *thread = pool->threads[pool->next_thread];
    pool->next_thread = (pool->next_thread + 1) % pool->thread_count;
    pthread_mutex_unlock(&pool->mutex);
    
    return thread;
}

// 添加连接到 IO 线程
int io_thread_add_connection(io_thread_t *io_thread, int client_fd, struct sockaddr_in *addr) {
    if (!io_thread || client_fd < 0) return -1;
    
    // 创建连接对象
    connection_t *conn = (connection_t*)malloc(sizeof(connection_t));
    if (!conn) return -1;
    
    conn->fd = client_fd;
    conn->state = CONN_STATE_CONNECTED;
    conn->read_pos = 0;
    conn->write_pos = 0;
    conn->write_size = 0;
    conn->last_active = time(NULL);
    if (addr) {
        conn->addr = *addr;
    }
    
    // 通过管道发送连接指针到 IO 线程
    if (write(io_thread->pipe_fd[1], &conn, sizeof(conn)) != sizeof(conn)) {
        free(conn);
        return -1;
    }
    
    return 0;
}