// epoll_wrapper.h
#ifndef EPOLL_WRAPPER_H
#define EPOLL_WRAPPER_H

#include "common.h"

typedef struct epoll_wrapper {
    int epfd;
    struct epoll_event *events;
    int max_events;
} epoll_wrapper_t;

// 创建 epoll 实例
epoll_wrapper_t* epoll_wrapper_create(int max_events);

// 销毁 epoll 实例
void epoll_wrapper_destroy(epoll_wrapper_t *epoll);

// 添加文件描述符到 epoll
int epoll_wrapper_add(epoll_wrapper_t *epoll, int fd, uint32_t events, void *ptr);

// 修改 epoll 中的文件描述符
int epoll_wrapper_mod(epoll_wrapper_t *epoll, int fd, uint32_t events, void *ptr);

// 从 epoll 中删除文件描述符
int epoll_wrapper_del(epoll_wrapper_t *epoll, int fd);

// 等待事件
int epoll_wrapper_wait(epoll_wrapper_t *epoll, int timeout);

#endif // EPOLL_WRAPPER_H