#include <stdlib.h>
#include "event_loop.h"

// Platform-specific backend selection
#ifdef __linux__
extern const event_loop_ops_t epoll_ops;
#elif defined(__APPLE__)
extern const event_loop_ops_t kqueue_ops;
#else
#error "Unsupported platform"
#endif

event_loop_t* event_loop_create(int max_events) {
    event_loop_t *loop = calloc(1, sizeof(event_loop_t));
    if (!loop) return NULL;
    
#ifdef __linux__
    loop->ops = &epoll_ops;
#elif defined(__APPLE__)
    loop->ops = &kqueue_ops;
#endif
    
    if (loop->ops->create(loop, max_events) < 0) {
        free(loop);
        return NULL;
    }
    
    return loop;
}

void event_loop_destroy(event_loop_t *loop) {
    if (!loop) return;
    
    if (loop->ops && loop->ops->destroy) {
        loop->ops->destroy(loop);
    }
    
    free(loop);
}

int event_loop_add(event_loop_t *loop, int fd, uint32_t events, void *data) {
    if (!loop || !loop->ops || !loop->ops->add) return -1;
    return loop->ops->add(loop, fd, events, data);
}

int event_loop_mod(event_loop_t *loop, int fd, uint32_t events, void *data) {
    if (!loop || !loop->ops || !loop->ops->mod) return -1;
    return loop->ops->mod(loop, fd, events, data);
}

int event_loop_del(event_loop_t *loop, int fd) {
    if (!loop || !loop->ops || !loop->ops->del) return -1;
    return loop->ops->del(loop, fd);
}

int event_loop_wait(event_loop_t *loop, event_t *events, int max_events, int timeout) {
    if (!loop || !loop->ops || !loop->ops->wait) return -1;
    return loop->ops->wait(loop, events, max_events, timeout);
}