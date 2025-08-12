// conn_map.c
#include <stdlib.h>
#include <string.h>
#include "conn_map.h"

void conn_map_init(conn_map_t* map) {
    map->head = NULL;
}

void conn_map_set(conn_map_t* map, int fd, const char* buf) {
    conn_entry_t* cur = map->head;
    while (cur) {
        if (cur->fd == fd) {
            free(cur->buf);
            cur->buf = strdup(buf);
            return;
        }
        cur = cur->next;
    }

    // 未找到，插入新节点
    conn_entry_t* entry = malloc(sizeof(conn_entry_t));
    entry->fd = fd;
    entry->buf = strdup(buf);
    entry->next = map->head;
    map->head = entry;
}

char* conn_map_get(conn_map_t* map, int fd) {
    conn_entry_t* cur = map->head;
    while (cur) {
        if (cur->fd == fd) {
            return cur->buf;
        }
        cur = cur->next;
    }
    return NULL;
}

void conn_map_remove(conn_map_t* map, int fd) {
    conn_entry_t *prev = NULL, *cur = map->head;
    while (cur) {
        if (cur->fd == fd) {
            if (prev) {
                prev->next = cur->next;
            } else {
                map->head = cur->next;
            }
            free(cur->buf);
            free(cur);
            return;
        }
        prev = cur;
        cur = cur->next;
    }
}