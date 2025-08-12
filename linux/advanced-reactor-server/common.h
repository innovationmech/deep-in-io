#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <signal.h>
#include <assert.h>
#include <time.h>

#define MAX_EVENTS 1024
#define BUFFER_SIZE 4096
#define BACKLOG 128
#define MAX_THREADS 16

// 连接状态
typedef enum {
    CONN_STATE_CONNECTED,
    CONN_STATE_READING,
    CONN_STATE_WRITING,
    CONN_STATE_CLOSING
} conn_state_t;

// 连接结构体
typedef struct connection {
    int fd;
    int epoll_fd;           // 所属的 epoll 实例
    conn_state_t state;
    char read_buf[BUFFER_SIZE];
    char write_buf[BUFFER_SIZE];
    int read_pos;
    int write_pos;
    int write_size;
    struct sockaddr_in addr;
    time_t last_active;
    void *io_thread;        // 所属的 IO 线程
} connection_t;

// 任务类型
typedef enum {
    TASK_TYPE_READ,
    TASK_TYPE_WRITE,
    TASK_TYPE_PROCESS,
    TASK_TYPE_CLOSE
} task_type_t;

// 任务结构体
typedef struct task {
    task_type_t type;
    connection_t *conn;
    void *data;
    int data_len;
    struct task *next;
} task_t;

// 设置非阻塞
static inline int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// 错误处理宏
#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define log_info(fmt, ...) \
    printf("[INFO] " fmt "\n", ##__VA_ARGS__)

#define log_error(fmt, ...) \
    fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)

#endif // COMMON_H