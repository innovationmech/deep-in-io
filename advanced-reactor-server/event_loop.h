#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include <stdint.h>
#include <sys/types.h>

// Platform-agnostic event types
#define EVENT_READ    0x01
#define EVENT_WRITE   0x02
#define EVENT_ERROR   0x04
#define EVENT_HUP     0x08
#define EVENT_RDHUP   0x10
#define EVENT_ET      0x20  // Edge-triggered mode

// Forward declaration
typedef struct event_loop event_loop_t;

// Event structure
typedef struct {
    void *data;      // User data pointer
    uint32_t events; // Event mask
    int fd;          // File descriptor
} event_t;

// Event loop operations
typedef struct event_loop_ops {
    int (*create)(event_loop_t *loop, int max_events);
    void (*destroy)(event_loop_t *loop);
    int (*add)(event_loop_t *loop, int fd, uint32_t events, void *data);
    int (*mod)(event_loop_t *loop, int fd, uint32_t events, void *data);
    int (*del)(event_loop_t *loop, int fd);
    int (*wait)(event_loop_t *loop, event_t *events, int max_events, int timeout);
} event_loop_ops_t;

// Event loop structure
struct event_loop {
    void *impl;                  // Platform-specific implementation
    const event_loop_ops_t *ops; // Operation table
    int max_events;              // Maximum events to handle
};

// Public API
event_loop_t* event_loop_create(int max_events);
void event_loop_destroy(event_loop_t *loop);
int event_loop_add(event_loop_t *loop, int fd, uint32_t events, void *data);
int event_loop_mod(event_loop_t *loop, int fd, uint32_t events, void *data);
int event_loop_del(event_loop_t *loop, int fd);
int event_loop_wait(event_loop_t *loop, event_t *events, int max_events, int timeout);

// Platform-specific implementations (defined in event_loop_epoll.c or event_loop_kqueue.c)
extern const event_loop_ops_t epoll_ops;
extern const event_loop_ops_t kqueue_ops;

#endif // EVENT_LOOP_H