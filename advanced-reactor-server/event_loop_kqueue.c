#ifdef __APPLE__

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include "event_loop.h"

typedef struct {
    int kq;
    struct kevent *events;
    struct kevent *changes;
    int nchanges;
    int max_changes;
} kqueue_impl_t;

static int kqueue_create_impl(event_loop_t *loop, int max_events) {
    kqueue_impl_t *impl = calloc(1, sizeof(kqueue_impl_t));
    if (!impl) return -1;
    
    impl->kq = kqueue();
    if (impl->kq == -1) {
        free(impl);
        return -1;
    }
    
    impl->events = calloc(max_events, sizeof(struct kevent));
    if (!impl->events) {
        close(impl->kq);
        free(impl);
        return -1;
    }
    
    impl->max_changes = max_events;
    impl->changes = calloc(max_events, sizeof(struct kevent));
    if (!impl->changes) {
        free(impl->events);
        close(impl->kq);
        free(impl);
        return -1;
    }
    
    impl->nchanges = 0;
    loop->impl = impl;
    loop->max_events = max_events;
    return 0;
}

static void kqueue_destroy_impl(event_loop_t *loop) {
    if (!loop || !loop->impl) return;
    
    kqueue_impl_t *impl = (kqueue_impl_t *)loop->impl;
    if (impl->kq >= 0) {
        close(impl->kq);
    }
    if (impl->events) {
        free(impl->events);
    }
    if (impl->changes) {
        free(impl->changes);
    }
    free(impl);
    loop->impl = NULL;
}

static int kqueue_add_impl(event_loop_t *loop, int fd, uint32_t events, void *data) {
    if (!loop || !loop->impl || fd < 0) return -1;
    
    kqueue_impl_t *impl = (kqueue_impl_t *)loop->impl;
    
    // Clear pending changes for this fd
    impl->nchanges = 0;
    
    if (events & EVENT_READ) {
        if (impl->nchanges >= impl->max_changes) return -1;
        int flags = EV_ADD | EV_ENABLE;
        if (events & EVENT_ET) flags |= EV_CLEAR;  // Edge-triggered
        EV_SET(&impl->changes[impl->nchanges++], fd, EVFILT_READ, flags, 0, 0, data);
    }
    
    if (events & EVENT_WRITE) {
        if (impl->nchanges >= impl->max_changes) return -1;
        int flags = EV_ADD | EV_ENABLE;
        if (events & EVENT_ET) flags |= EV_CLEAR;  // Edge-triggered
        EV_SET(&impl->changes[impl->nchanges++], fd, EVFILT_WRITE, flags, 0, 0, data);
    }
    
    // Apply changes immediately
    struct timespec timeout = {0, 0};
    int ret = kevent(impl->kq, impl->changes, impl->nchanges, NULL, 0, &timeout);
    impl->nchanges = 0;
    return ret < 0 ? -1 : 0;
}

static int kqueue_mod_impl(event_loop_t *loop, int fd, uint32_t events, void *data) {
    // For kqueue, modification is the same as add with EV_ADD flag
    return kqueue_add_impl(loop, fd, events, data);
}

static int kqueue_del_impl(event_loop_t *loop, int fd) {
    if (!loop || !loop->impl || fd < 0) return -1;
    
    kqueue_impl_t *impl = (kqueue_impl_t *)loop->impl;
    impl->nchanges = 0;
    
    // Delete both read and write filters
    if (impl->nchanges < impl->max_changes) {
        EV_SET(&impl->changes[impl->nchanges++], fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    }
    if (impl->nchanges < impl->max_changes) {
        EV_SET(&impl->changes[impl->nchanges++], fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
    }
    
    // Apply changes immediately
    struct timespec timeout = {0, 0};
    int ret = kevent(impl->kq, impl->changes, impl->nchanges, NULL, 0, &timeout);
    impl->nchanges = 0;
    return ret < 0 ? -1 : 0;
}

static int kqueue_wait_impl(event_loop_t *loop, event_t *events, int max_events, int timeout) {
    if (!loop || !loop->impl || !events) return -1;
    
    kqueue_impl_t *impl = (kqueue_impl_t *)loop->impl;
    struct timespec ts, *pts = NULL;
    
    if (timeout >= 0) {
        ts.tv_sec = timeout / 1000;
        ts.tv_nsec = (timeout % 1000) * 1000000;
        pts = &ts;
    }
    
    int n = kevent(impl->kq, NULL, 0, impl->events, max_events, pts);
    
    if (n > 0) {
        for (int i = 0; i < n; i++) {
            events[i].data = impl->events[i].udata;
            events[i].fd = impl->events[i].ident;
            events[i].events = 0;
            
            if (impl->events[i].filter == EVFILT_READ) {
                events[i].events |= EVENT_READ;
            }
            if (impl->events[i].filter == EVFILT_WRITE) {
                events[i].events |= EVENT_WRITE;
            }
            if (impl->events[i].flags & EV_EOF) {
                events[i].events |= EVENT_HUP;
            }
            if (impl->events[i].flags & EV_ERROR) {
                events[i].events |= EVENT_ERROR;
            }
        }
    }
    
    return n;
}

const event_loop_ops_t kqueue_ops = {
    .create = kqueue_create_impl,
    .destroy = kqueue_destroy_impl,
    .add = kqueue_add_impl,
    .mod = kqueue_mod_impl,
    .del = kqueue_del_impl,
    .wait = kqueue_wait_impl
};

#endif // __APPLE__