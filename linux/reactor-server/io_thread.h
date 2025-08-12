// io_thread.h
#ifndef IO_THREAD_H
#define IO_THREAD_H

#include <pthread.h>
#include "conn_map.h"

typedef struct io_thread_t {
    pthread_t thread;
    int epoll_fd;
    int index;
    conn_map_t conn_map;
} io_thread_t;

void io_thread_start(io_thread_t* io, int index);
void io_thread_add_conn(io_thread_t* io, int fd);
void io_thread_handle_response(io_thread_t* io, int fd, const char* response);

#endif // IO_THREAD_H