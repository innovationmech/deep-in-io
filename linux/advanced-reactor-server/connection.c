// connection.c
#include "common.h"

// 创建连接对象
connection_t* conn_create(int fd, int epoll_fd, struct sockaddr_in *addr, void *io_thread) {
    connection_t *conn = (connection_t*)malloc(sizeof(connection_t));
    if (!conn) return NULL;
    
    conn->fd = fd;
    conn->epoll_fd = epoll_fd;
    conn->state = CONN_STATE_CONNECTED;
    conn->read_pos = 0;
    conn->write_pos = 0;
    conn->write_size = 0;
    if (addr) {
        conn->addr = *addr;
    } else {
        memset(&conn->addr, 0, sizeof(conn->addr));
    }
    conn->last_active = time(NULL);
    conn->io_thread = io_thread;
    conn->ref_count = 1;  // 初始引用计数为1
    conn->closing = 0;
    
    // 初始化互斥锁
    if (pthread_mutex_init(&conn->conn_mutex, NULL) != 0) {
        free(conn);
        return NULL;
    }
    
    return conn;
}

// 增加连接引用计数
void conn_acquire(connection_t *conn) {
    if (!conn) return;
    
    pthread_mutex_lock(&conn->conn_mutex);
    conn->ref_count++;
    pthread_mutex_unlock(&conn->conn_mutex);
}

// 减少连接引用计数，当计数为0时释放连接
void conn_release(connection_t *conn) {
    if (!conn) return;
    
    pthread_mutex_lock(&conn->conn_mutex);
    conn->ref_count--;
    int should_free = (conn->ref_count <= 0);
    pthread_mutex_unlock(&conn->conn_mutex);
    
    if (should_free) {
        // 确保文件描述符已关闭
        if (conn->fd >= 0) {
            close(conn->fd);
            conn->fd = -1;
        }
        
        pthread_mutex_destroy(&conn->conn_mutex);
        free(conn);
    }
}

// 检查连接是否有效（未被标记为关闭）
int conn_is_valid(connection_t *conn) {
    if (!conn) return 0;
    
    pthread_mutex_lock(&conn->conn_mutex);
    int valid = !conn->closing && conn->state != CONN_STATE_CLOSED;
    pthread_mutex_unlock(&conn->conn_mutex);
    
    return valid;
}

// 标记连接为正在关闭
void conn_mark_closing(connection_t *conn) {
    if (!conn) return;
    
    pthread_mutex_lock(&conn->conn_mutex);
    conn->closing = 1;
    conn->state = CONN_STATE_CLOSING;
    pthread_mutex_unlock(&conn->conn_mutex);
}