// server.c
#include "server.h"

// 信号处理
static volatile int g_shutdown = 0;

static void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        log_info("Received shutdown signal");
        g_shutdown = 1;
    }
}

// 创建监听套接字
static int create_listen_socket(int port) {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        handle_error("socket");
    }
    
    // 设置套接字选项
    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        handle_error("setsockopt SO_REUSEADDR");
    }
    
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
        log_error("setsockopt SO_REUSEPORT failed: %s", strerror(errno));
    }
    
    // 绑定地址
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    
    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        handle_error("bind");
    }
    
    // 监听
    if (listen(listen_fd, BACKLOG) == -1) {
        handle_error("listen");
    }
    
    // 设置非阻塞
    set_nonblocking(listen_fd);
    
    return listen_fd;
}

// 接受新连接
static void accept_connections(reactor_server_t *server) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd;
    
    while (1) {
        client_fd = accept(server->listen_fd, (struct sockaddr*)&client_addr, &addr_len);
        
        if (client_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 没有更多连接
                break;
            } else if (errno == EINTR) {
                // 被信号中断，继续
                continue;
            } else {
                log_error("accept error: %s", strerror(errno));
                break;
            }
        }
        
        // 设置客户端套接字为非阻塞
        set_nonblocking(client_fd);
        
        // 设置 TCP_NODELAY
        int opt = 1;
        setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
        
        // 获取 IO 线程（轮询）
        io_thread_t *io_thread = io_thread_pool_get_thread(server->io_pool);
        if (!io_thread) {
            log_error("No IO thread available");
            close(client_fd);
            continue;
        }
        
        // 将连接分配给 IO 线程
        if (io_thread_add_connection(io_thread, client_fd, &client_addr) == 0) {
            pthread_mutex_lock(&server->stats_mutex);
            server->total_connections++;
            pthread_mutex_unlock(&server->stats_mutex);
            
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
            log_info("New connection from %s:%d, assigned to IO thread %d", 
                    client_ip, ntohs(client_addr.sin_port), io_thread->thread_index);
        } else {
            log_error("Failed to add connection to IO thread");
            close(client_fd);
        }
    }
}

reactor_server_t* server_create(int port, int io_threads, int worker_threads) {
    reactor_server_t *server = (reactor_server_t*)malloc(sizeof(reactor_server_t));
    if (!server) return NULL;
    
    server->port = port;
    server->running = 0;
    server->total_connections = 0;
    pthread_mutex_init(&server->stats_mutex, NULL);
    
    // 创建监听套接字
    server->listen_fd = create_listen_socket(port);
    
    // 创建工作线程池
    server->worker_pool = thread_pool_create(worker_threads, 2000);
    if (!server->worker_pool) {
        close(server->listen_fd);
        free(server);
        return NULL;
    }
    
    // 创建 IO 线程池
    server->io_pool = io_thread_pool_create(io_threads, server->worker_pool);
    if (!server->io_pool) {
        thread_pool_destroy(server->worker_pool);
        close(server->listen_fd);
        free(server);
        return NULL;
    }
    
    // 创建主线程的 epoll
    server->main_epoll = epoll_wrapper_create(10);
    if (!server->main_epoll) {
        io_thread_pool_destroy(server->io_pool);
        thread_pool_destroy(server->worker_pool);
        close(server->listen_fd);
        free(server);
        return NULL;
    }
    
    // 将监听套接字添加到主 epoll
    if (epoll_wrapper_add(server->main_epoll, server->listen_fd, 
                         EPOLLIN | EPOLLET, NULL) == -1) {
        epoll_wrapper_destroy(server->main_epoll);
        io_thread_pool_destroy(server->io_pool);
        thread_pool_destroy(server->worker_pool);
        close(server->listen_fd);
        free(server);
        return NULL;
    }
    
    log_info("Server created: port=%d, io_threads=%d, worker_threads=%d", 
            port, io_threads, worker_threads);
    
    return server;
}

int server_start(reactor_server_t *server) {
    if (!server) return -1;
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN);
    
    server->running = 1;
    log_info("Server starting on port %d...", server->port);
    
    // 主循环：只处理 accept
    while (server->running && !g_shutdown) {
        int nfds = epoll_wrapper_wait(server->main_epoll, 1);
        
        if (nfds == -1) {
            if (errno == EINTR) continue;
            log_error("epoll_wait error: %s", strerror(errno));
            break;
        }
        
        for (int i = 0; i < nfds; i++) {
            struct epoll_event *ev = &server->main_epoll->events[i];
            
            if (ev->events & EPOLLIN) {
                accept_connections(server);
            }
            
            if (ev->events & (EPOLLERR | EPOLLHUP)) {
                log_error("Error on listen socket");
                server->running = 0;
                break;
            }
        }
    }
    
    log_info("Server stopped. Total connections handled: %ld", server->total_connections);
    
    return 0;
}

void server_stop(reactor_server_t *server) {
    if (!server) return;
    server->running = 0;
}

void server_destroy(reactor_server_t *server) {
    if (!server) return;
    
    // 关闭监听套接字
    if (server->listen_fd != -1) {
        close(server->listen_fd);
    }
    
    // 销毁各组件
    epoll_wrapper_destroy(server->main_epoll);
    io_thread_pool_destroy(server->io_pool);
    thread_pool_destroy(server->worker_pool);
    
    pthread_mutex_destroy(&server->stats_mutex);
    
    free(server);
    
    log_info("Server destroyed");
}