#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>

#define PORT 8888
#define WORKER_COUNT 4
#define MAX_EVENTS 1024
#define BUFFER_SIZE 1024

typedef struct {
    int epoll_fd;
    int pipe_fd[2];  // [0] for read by worker, [1] for write by main thread
    pthread_t thread_id;
} Worker;

// Global array of all workers
Worker workers[WORKER_COUNT];
int current_worker = 0;

// Set socket to non-blocking
int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// Main epoll loop for worker threads
void *worker_loop(void *arg) {
    Worker *w = (Worker *)arg;
    struct epoll_event events[MAX_EVENTS];

    printf("Worker thread %ld started\n", w->thread_id);

    while (1) {
        int n = epoll_wait(w->epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;

            if (fd == w->pipe_fd[0]) {
                // Receive connection assigned by main thread
                int client_fd;
                read(w->pipe_fd[0], &client_fd, sizeof(client_fd));
                set_nonblocking(client_fd);

                struct epoll_event ev;
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = client_fd;
                epoll_ctl(w->epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);

                printf("Worker %ld received new fd: %d\n", w->thread_id, client_fd);
            } else {
                // Client data arrived
                char buffer[BUFFER_SIZE];
                int nread = read(fd, buffer, BUFFER_SIZE);
                if (nread <= 0) {
                    printf("Client disconnected: fd=%d\n", fd);
                    close(fd);
                    epoll_ctl(w->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                } else {
                    buffer[nread] = '\0';
                    printf("Worker %ld received from fd %d: %s\n", w->thread_id, fd, buffer);
                }
            }
        }
    }

    return NULL;
}

// Start worker thread pool
void start_workers() {
    for (int i = 0; i < WORKER_COUNT; ++i) {
        pipe(workers[i].pipe_fd);  // Pipe communication
        workers[i].epoll_fd = epoll_create1(0);

        // Add pipe read end to epoll to receive connections assigned by main thread
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = workers[i].pipe_fd[0];
        epoll_ctl(workers[i].epoll_fd, EPOLL_CTL_ADD, workers[i].pipe_fd[0], &ev);

        pthread_create(&workers[i].thread_id, NULL, worker_loop, &workers[i]);
    }
}

// Main thread: listen + distribute connections
int main() {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    set_nonblocking(listen_fd);

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(PORT)
    };

    bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(listen_fd, 128);

    printf("Main thread listening on port %d...\n", PORT);

    // Start worker threads
    start_workers();

    // Accept new connections and distribute to workers
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int conn_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &len);
        if (conn_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            } else {
                perror("accept");
                break;
            }
        }

        // Distribute to next worker
        Worker *w = &workers[current_worker];
        write(w->pipe_fd[1], &conn_fd, sizeof(conn_fd));
        current_worker = (current_worker + 1) % WORKER_COUNT;
    }

    close(listen_fd);
    return 0;
}