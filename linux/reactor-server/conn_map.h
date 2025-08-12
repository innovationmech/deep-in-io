// conn_map.h
#ifndef CONN_MAP_H
#define CONN_MAP_H

typedef struct conn_entry {
    int fd;
    char* buf; // 可写内容（堆内存）
    struct conn_entry* next;
} conn_entry_t;

typedef struct conn_map {
    conn_entry_t* head;
} conn_map_t;

void conn_map_init(conn_map_t* map);

// 添加或更新写缓冲
void conn_map_set(conn_map_t* map, int fd, const char* buf);

// 获取写缓冲
char* conn_map_get(conn_map_t* map, int fd);

// 删除缓冲记录（写完）
void conn_map_remove(conn_map_t* map, int fd);

#endif // CONN_MAP_H