// io_thread.c
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <string.h>
#include <errno.h>
#include "io_thread.h"
#include "util.h"
#include "global_queue.h"

#define MAX_EVENTS 64
#define BUF_SIZE 4096

static void* io_thread_loop(void* arg) {
    io_thread_t* io = (io_thread_t*)arg;
    struct epoll_event events[MAX_EVENTS];
    conn_map_init(&io->conn_map);

    printf("[IO Thread %d] Started\n", io->index);

    while (1) {
        int n = epoll_wait(io->epoll_fd, events, MAX_EVENTS, 10);  // 10ms 超时
        if (n < 0 && errno != EINTR) {
            perror("epoll_wait");
            continue;
        }

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            printf("[IO Thread %d] Got epoll event on fd=%d, events=0x%x\n", io->index, fd, events[i].events);

            // 读取数据：循环 read 直到 EAGAIN（必须满足 EPOLLET）
            if (events[i].events & EPOLLIN) {
                char buffer[BUF_SIZE];
                
                while (1) {
                    int len = read(fd, buffer, sizeof(buffer) - 1);
                    if (len > 0) {
                        buffer[len] = '\0';
                        printf("[IO Thread %d] Read %d bytes from fd=%d: %s", io->index, len, fd, buffer);
                        
                        // 直接加入全局队列，避免复杂的内存管理
                        global_queue_add_task(fd, buffer);
                        
                    } else if (len == 0) {
                        // 客户端关闭连接
                        printf("[IO Thread %d] Client fd=%d disconnected\n", io->index, fd);
                        close(fd);
                        epoll_ctl(io->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                        conn_map_remove(&io->conn_map, fd);
                        break;
                    } else {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break; // 读取完毕
                        } else {
                            perror("read");
                            close(fd);
                            epoll_ctl(io->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                            conn_map_remove(&io->conn_map, fd);
                            break;
                        }
                    }
                }
            }

            // 写事件触发
            if (events[i].events & EPOLLOUT) {
                char* out = conn_map_get(&io->conn_map, fd);
                if (out) {
                    int len = write(fd, out, strlen(out));
                    if (len >= 0) {
                        // 写完后取消 EPOLLOUT
                        epoll_ctl(io->epoll_fd, EPOLL_CTL_MOD, fd, &(struct epoll_event){
                            .events = EPOLLIN | EPOLLET,
                            .data.fd = fd
                        });
                        conn_map_remove(&io->conn_map, fd);
                    } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        perror("write");
                        close(fd);
                        conn_map_remove(&io->conn_map, fd);
                    }
                }
            }
        }
    }

    return NULL;
}

void io_thread_start(io_thread_t* io, int index) {
    io->index = index;
    io->epoll_fd = epoll_create1(0);
    if (io->epoll_fd < 0) die("epoll_create");

    pthread_create(&io->thread, NULL, io_thread_loop, io);
}

void io_thread_add_conn(io_thread_t* io, int fd) {
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = fd;
    if (epoll_ctl(io->epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        perror("epoll_ctl ADD");
        close(fd);
    }
}

void io_thread_handle_response(io_thread_t* io, int fd, const char* response) {
    // 设置写缓冲
    conn_map_set(&io->conn_map, fd, response);

    // 注册 EPOLLOUT 事件
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
    ev.data.fd = fd;
    if (epoll_ctl(io->epoll_fd, EPOLL_CTL_MOD, fd, &ev) < 0) {
        perror("epoll_ctl MOD (response)");
    }
}