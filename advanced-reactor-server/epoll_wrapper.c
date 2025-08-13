#include "epoll_wrapper.h"

epoll_wrapper_t* epoll_wrapper_create(int max_events) {
    epoll_wrapper_t *epoll = (epoll_wrapper_t*)malloc(sizeof(epoll_wrapper_t));
    if (!epoll) return NULL;
    
    epoll->epfd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll->epfd == -1) {
        free(epoll);
        return NULL;
    }
    
    epoll->max_events = max_events;
    epoll->events = (struct epoll_event*)calloc(max_events, sizeof(struct epoll_event));
    if (!epoll->events) {
        close(epoll->epfd);
        free(epoll);
        return NULL;
    }
    
    return epoll;
}

void epoll_wrapper_destroy(epoll_wrapper_t *epoll) {
    if (!epoll) return;
    
    if (epoll->epfd != -1) {
        close(epoll->epfd);
    }
    
    if (epoll->events) {
        free(epoll->events);
    }
    
    free(epoll);
}

int epoll_wrapper_add(epoll_wrapper_t *epoll, int fd, uint32_t events, void *ptr) {
    if (!epoll || fd < 0) return -1;
    
    struct epoll_event ev;
    ev.events = events;
    ev.data.ptr = ptr;
    
    return epoll_ctl(epoll->epfd, EPOLL_CTL_ADD, fd, &ev);
}

int epoll_wrapper_mod(epoll_wrapper_t *epoll, int fd, uint32_t events, void *ptr) {
    if (!epoll || fd < 0) return -1;
    
    struct epoll_event ev;
    ev.events = events;
    ev.data.ptr = ptr;
    
    return epoll_ctl(epoll->epfd, EPOLL_CTL_MOD, fd, &ev);
}

int epoll_wrapper_del(epoll_wrapper_t *epoll, int fd) {
    if (!epoll || fd < 0) return -1;
    
    return epoll_ctl(epoll->epfd, EPOLL_CTL_DEL, fd, NULL);
}

int epoll_wrapper_wait(epoll_wrapper_t *epoll, int timeout) {
    if (!epoll) return -1;
    
    return epoll_wait(epoll->epfd, epoll->events, epoll->max_events, timeout);
}