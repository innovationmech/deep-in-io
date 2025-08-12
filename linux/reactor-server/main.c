// main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#include "util.h"
#include "thread_pool.h"
#include "io_thread.h"
#include "global_queue.h"

#define PORT 8888
#define MAX_IO_THREADS 4
#define MAX_WORKER_THREADS 1

int main() {
    // 创建监听 socket
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) die("socket");

    // 设置 SO_REUSEADDR
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 绑定端口
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY,
    };
    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        die("bind");

    if (listen(listen_fd, SOMAXCONN) < 0)
        die("listen");

    set_nonblocking(listen_fd);

    printf("Server listening on port %d...\n", PORT);

    // 初始化全局队列
    global_queue_init();

    // 初始化工作线程池
    thread_pool_t worker_pool;
    thread_pool_init(&worker_pool, MAX_WORKER_THREADS);

    // 初始化多个 IO 线程
    io_thread_t io_threads[MAX_IO_THREADS];
    for (int i = 0; i < MAX_IO_THREADS; ++i) {
        io_thread_start(&io_threads[i], i);
    }

    // 主线程 accept 并分发连接给 IO 线程
    int current = 0;
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(1000); // 减少空转
                continue;
            } else {
                perror("accept");
                continue;
            }
        }

        set_nonblocking(client_fd);
        int target = current++ % MAX_IO_THREADS;
        printf("[Main] New connection fd=%d assigned to IO thread %d\n", client_fd, target);
        io_thread_add_conn(&io_threads[target], client_fd);
    }

    close(listen_fd);
    return 0;
}