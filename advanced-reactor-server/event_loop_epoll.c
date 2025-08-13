#ifdef __linux__

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include "event_loop.h"

typedef struct {
    int epfd;
    struct epoll_event *events;
} epoll_impl_t;

// Convert generic events to epoll events
static uint32_t to_epoll_events(uint32_t events) {
    uint32_t epoll_events = 0;
    if (events & EVENT_READ) epoll_events |= EPOLLIN;
    if (events & EVENT_WRITE) epoll_events |= EPOLLOUT;
    if (events & EVENT_ERROR) epoll_events |= EPOLLERR;
    if (events & EVENT_HUP) epoll_events |= EPOLLHUP;
    if (events & EVENT_RDHUP) epoll_events |= EPOLLRDHUP;
    if (events & EVENT_ET) epoll_events |= EPOLLET;
    return epoll_events;
}

// Convert epoll events to generic events
static uint32_t from_epoll_events(uint32_t epoll_events) {
    uint32_t events = 0;
    if (epoll_events & EPOLLIN) events |= EVENT_READ;
    if (epoll_events & EPOLLOUT) events |= EVENT_WRITE;
    if (epoll_events & EPOLLERR) events |= EVENT_ERROR;
    if (epoll_events & EPOLLHUP) events |= EVENT_HUP;
    if (epoll_events & EPOLLRDHUP) events |= EVENT_RDHUP;
    return events;
}

static int epoll_create_impl(event_loop_t *loop, int max_events) {
    epoll_impl_t *impl = calloc(1, sizeof(epoll_impl_t));
    if (!impl) return -1;
    
    impl->epfd = epoll_create1(EPOLL_CLOEXEC);
    if (impl->epfd == -1) {
        free(impl);
        return -1;
    }
    
    impl->events = calloc(max_events, sizeof(struct epoll_event));
    if (!impl->events) {
        close(impl->epfd);
        free(impl);
        return -1;
    }
    
    loop->impl = impl;
    loop->max_events = max_events;
    return 0;
}

static void epoll_destroy_impl(event_loop_t *loop) {
    if (!loop || !loop->impl) return;
    
    epoll_impl_t *impl = (epoll_impl_t *)loop->impl;
    if (impl->epfd >= 0) {
        close(impl->epfd);
    }
    if (impl->events) {
        free(impl->events);
    }
    free(impl);
    loop->impl = NULL;
}

static int epoll_add_impl(event_loop_t *loop, int fd, uint32_t events, void *data) {
    if (!loop || !loop->impl || fd < 0) return -1;
    
    epoll_impl_t *impl = (epoll_impl_t *)loop->impl;
    struct epoll_event ev;
    ev.events = to_epoll_events(events);
    ev.data.ptr = data;
    
    return epoll_ctl(impl->epfd, EPOLL_CTL_ADD, fd, &ev);
}

static int epoll_mod_impl(event_loop_t *loop, int fd, uint32_t events, void *data) {
    if (!loop || !loop->impl || fd < 0) return -1;
    
    epoll_impl_t *impl = (epoll_impl_t *)loop->impl;
    struct epoll_event ev;
    ev.events = to_epoll_events(events);
    ev.data.ptr = data;
    
    return epoll_ctl(impl->epfd, EPOLL_CTL_MOD, fd, &ev);
}

static int epoll_del_impl(event_loop_t *loop, int fd) {
    if (!loop || !loop->impl || fd < 0) return -1;
    
    epoll_impl_t *impl = (epoll_impl_t *)loop->impl;
    return epoll_ctl(impl->epfd, EPOLL_CTL_DEL, fd, NULL);
}

static int epoll_wait_impl(event_loop_t *loop, event_t *events, int max_events, int timeout) {
    if (!loop || !loop->impl || !events) return -1;
    
    epoll_impl_t *impl = (epoll_impl_t *)loop->impl;
    int n = epoll_wait(impl->epfd, impl->events, max_events, timeout);
    
    if (n > 0) {
        for (int i = 0; i < n; i++) {
            events[i].data = impl->events[i].data.ptr;
            events[i].events = from_epoll_events(impl->events[i].events);
            // epoll doesn't provide fd directly, store it in data or handle differently
            events[i].fd = -1;
        }
    }
    
    return n;
}

const event_loop_ops_t epoll_ops = {
    .create = epoll_create_impl,
    .destroy = epoll_destroy_impl,
    .add = epoll_add_impl,
    .mod = epoll_mod_impl,
    .del = epoll_del_impl,
    .wait = epoll_wait_impl
};

#endif // __linux__