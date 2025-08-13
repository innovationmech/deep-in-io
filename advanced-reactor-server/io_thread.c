// io_thread.c
#include "io_thread.h"
#include "event_loop.h"

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
            
            // 更新连接状态
            conn->state = CONN_STATE_READING;
            
            // 创建处理任务并提交到工作线程池
            task_t *task = task_create(TASK_TYPE_PROCESS, conn, buffer, n);
            if (task) {
                thread_pool_submit(io_thread->worker_pool, task);
            } else {
                log_error("Failed to create task for fd=%d", conn->fd);
            }
            
            conn->last_active = time(NULL);
        } else if (n == 0) {
            // 连接关闭
            int conn_fd = conn->fd;
            log_info("Connection closed by client: fd=%d", conn_fd);
            conn_mark_closing(conn);
            event_loop_del(io_thread->event_loop, conn_fd);
            conn_release(conn);
            return;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 数据读取完毕
                break;
            } else {
                int conn_fd = conn->fd;
                log_error("Read error: %s", strerror(errno));
                conn_mark_closing(conn);
                event_loop_del(io_thread->event_loop, conn_fd);
                conn_release(conn);
                return;
            }
        }
    }
}

// 处理写事件
static void handle_write(io_thread_t *io_thread, connection_t *conn) {
    if (conn->write_size <= 0) {
        // 没有数据要写，切换回读模式
        event_loop_mod(io_thread->event_loop, conn->fd, EVENT_READ | EVENT_ET, conn);
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
                int conn_fd = conn->fd;
                log_error("Write error: %s", strerror(errno));
                conn_mark_closing(conn);
                event_loop_del(io_thread->event_loop, conn_fd);
                conn_release(conn);
                return;
            }
        }
    }
    
    // 数据写完，切换回读模式
    if (conn->write_pos >= conn->write_size) {
        conn->write_pos = 0;
        conn->write_size = 0;
        event_loop_mod(io_thread->event_loop, conn->fd, EVENT_READ | EVENT_ET, conn);
        conn->state = CONN_STATE_READING;
    }
}

// IO 线程主函数
static void* io_thread_run(void *arg) {
    io_thread_t *io_thread = (io_thread_t*)arg;
    
    log_info("IO thread %d started", io_thread->thread_index);
    
    event_t events[MAX_EVENTS];
    
    while (!io_thread->shutdown) {
        int nfds = event_loop_wait(io_thread->event_loop, events, MAX_EVENTS, 1);
        
        for (int i = 0; i < nfds; i++) {
            event_t *ev = &events[i];
            
            // 检查是否是管道事件（用于接收新连接）
            if (ev->data == &io_thread->pipe_fd[0]) {
                // 读取管道数据
                connection_t *new_conn;
                if (read(io_thread->pipe_fd[0], &new_conn, sizeof(new_conn)) == sizeof(new_conn)) {
                    // 将新连接添加到 epoll
                    new_conn->event_loop = io_thread->event_loop;
                    new_conn->io_thread = io_thread;
                    
                    if (event_loop_add(io_thread->event_loop, new_conn->fd, 
                                         EVENT_READ | EVENT_ET, new_conn) == 0) {
                        pthread_mutex_lock(&io_thread->stats_mutex);
                        io_thread->connections_handled++;
                        pthread_mutex_unlock(&io_thread->stats_mutex);
                        
                        log_info("IO thread %d: new connection fd=%d", 
                                io_thread->thread_index, new_conn->fd);
                    } else {
                        log_error("Failed to add connection to epoll");
                        conn_release(new_conn);
                    }
                }
                continue;
            }
            
            // 检查是否是消息管道事件
            if (ev->data == &io_thread->msg_pipe_fd[0]) {
                char dummy;
                read(io_thread->msg_pipe_fd[0], &dummy, 1);  // 清空管道
                
                // 处理消息队列中的所有消息
                pthread_mutex_lock(&io_thread->msg_queue_mutex);
                while (io_thread->msg_queue_head) {
                    io_message_t *msg = io_thread->msg_queue_head;
                    io_thread->msg_queue_head = msg->next;
                    if (!io_thread->msg_queue_head) {
                        io_thread->msg_queue_tail = NULL;
                    }
                    pthread_mutex_unlock(&io_thread->msg_queue_mutex);
                    
                    // 处理消息（在释放队列锁后检查连接有效性）
                    if (msg->type == IO_MSG_RESPONSE_READY) {
                        if (conn_is_valid(msg->conn)) {
                            event_loop_mod(io_thread->event_loop, msg->conn->fd, EVENT_WRITE | EVENT_ET, msg->conn);
                        }
                    }
                    
                    conn_release(msg->conn);
                    free(msg);
                    
                    pthread_mutex_lock(&io_thread->msg_queue_mutex);
                }
                pthread_mutex_unlock(&io_thread->msg_queue_mutex);
                continue;
            }
            
            connection_t *conn = (connection_t*)ev->data;
            if (!conn) continue;
            
            // Check if connection is already being closed to prevent double handling
            if (!conn_is_valid(conn)) {
                continue;
            }
            
            int conn_fd = conn->fd;  // Store fd before potential close
            
            if (ev->events & EVENT_READ) {
                handle_read(io_thread, conn);
                // Check if connection was closed during read
                if (!conn_is_valid(conn)) {
                    continue;
                }
            }
            
            if (ev->events & EVENT_WRITE) {
                handle_write(io_thread, conn);
                // Check if connection was closed during write
                if (!conn_is_valid(conn)) {
                    continue;
                }
            }
            
            if (ev->events & (EVENT_ERROR | EVENT_HUP)) {
                log_info("Connection error/hangup: fd=%d", conn_fd);
                conn_mark_closing(conn);
                event_loop_del(io_thread->event_loop, conn_fd);
                conn_release(conn);
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
    
    // 创建消息管道
    if (pipe(io_thread->msg_pipe_fd) == -1) {
        close(io_thread->pipe_fd[0]);
        close(io_thread->pipe_fd[1]);
        free(io_thread);
        return NULL;
    }
    
    // 设置管道为非阻塞
    set_nonblocking(io_thread->pipe_fd[0]);
    set_nonblocking(io_thread->pipe_fd[1]);
    set_nonblocking(io_thread->msg_pipe_fd[0]);
    set_nonblocking(io_thread->msg_pipe_fd[1]);
    
    // 初始化消息队列
    io_thread->msg_queue_head = NULL;
    io_thread->msg_queue_tail = NULL;
    pthread_mutex_init(&io_thread->msg_queue_mutex, NULL);
    
    pthread_mutex_init(&io_thread->stats_mutex, NULL);
    
    // 创建 event loop 实例
    io_thread->event_loop = event_loop_create(MAX_EVENTS);
    if (!io_thread->event_loop) {
        close(io_thread->pipe_fd[0]);
        close(io_thread->pipe_fd[1]);
        free(io_thread);
        return NULL;
    }
    
    // 将管道读端添加到 epoll
    if (event_loop_add(io_thread->event_loop, io_thread->pipe_fd[0], 
                         EVENT_READ | EVENT_ET, &io_thread->pipe_fd[0]) == -1) {
        event_loop_destroy(io_thread->event_loop);
        close(io_thread->pipe_fd[0]);
        close(io_thread->pipe_fd[1]);
        close(io_thread->msg_pipe_fd[0]);
        close(io_thread->msg_pipe_fd[1]);
        free(io_thread);
        return NULL;
    }
    
    // 将消息管道读端添加到 epoll
    if (event_loop_add(io_thread->event_loop, io_thread->msg_pipe_fd[0], 
                         EVENT_READ | EVENT_ET, &io_thread->msg_pipe_fd[0]) == -1) {
        event_loop_destroy(io_thread->event_loop);
        close(io_thread->pipe_fd[0]);
        close(io_thread->pipe_fd[1]);
        close(io_thread->msg_pipe_fd[0]);
        close(io_thread->msg_pipe_fd[1]);
        free(io_thread);
        return NULL;
    }
    
    // 创建线程
    if (pthread_create(&io_thread->thread_id, NULL, io_thread_run, io_thread) != 0) {
        event_loop_destroy(io_thread->event_loop);
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
    event_loop_destroy(io_thread->event_loop);
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
    
    // 创建连接对象 (event_loop will be set later in the IO thread)
    connection_t *conn = conn_create(client_fd, NULL, addr, io_thread);
    if (!conn) return -1;
    
    // 通过管道发送连接指针到 IO 线程
    if (write(io_thread->pipe_fd[1], &conn, sizeof(conn)) != sizeof(conn)) {
        conn_release(conn);
        return -1;
    }
    
    return 0;
}

// 向IO线程发送消息
void io_thread_send_message(io_thread_t *io_thread, io_msg_type_t type, connection_t *conn) {
    if (!io_thread || !conn) return;
    
    // 创建消息
    io_message_t *msg = (io_message_t*)malloc(sizeof(io_message_t));
    if (!msg) return;
    
    msg->type = type;
    msg->conn = conn;
    msg->next = NULL;
    
    // 将消息添加到队列（先获取队列锁，再增加引用计数）
    pthread_mutex_lock(&io_thread->msg_queue_mutex);
    
    // 增加连接引用计数
    conn_acquire(conn);
    
    if (io_thread->msg_queue_tail) {
        io_thread->msg_queue_tail->next = msg;
    } else {
        io_thread->msg_queue_head = msg;
    }
    io_thread->msg_queue_tail = msg;
    pthread_mutex_unlock(&io_thread->msg_queue_mutex);
    
    // 通知IO线程处理消息
    char dummy = 1;
    write(io_thread->msg_pipe_fd[1], &dummy, 1);
}