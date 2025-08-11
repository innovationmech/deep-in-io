#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

#define PORT 8888
#define MAX_EVENTS 10
#define BUFFER_SIZE 1024

// Set socket to non-blocking
int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    int listen_fd, conn_fd, epoll_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    char buffer[BUFFER_SIZE];

    struct epoll_event ev, events[MAX_EVENTS];

    // Create listening socket
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    set_nonblocking(listen_fd);

    // Bind address and port
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(listen_fd, 10);

    // Create epoll instance
    epoll_fd = epoll_create1(0);

    // Add listening socket to epoll
    ev.events = EPOLLIN;  // Monitor readable events
    ev.data.fd = listen_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev);

    printf("Epoll server is running on port %d...\n", PORT);

    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == listen_fd) {
                // New connection
                client_len = sizeof(client_addr);
                conn_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
                set_nonblocking(conn_fd);

                // Add new connection to epoll
                ev.events = EPOLLIN | EPOLLET;  // Edge triggered
                ev.data.fd = conn_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_fd, &ev);

                printf("New connection accepted: fd=%d\n", conn_fd);
            } else {
                // Data available to read
                int client_fd = events[i].data.fd;
                int n = read(client_fd, buffer, BUFFER_SIZE);
                if (n <= 0) {
                    printf("Client disconnected: fd=%d\n", client_fd);
                    close(client_fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                } else {
                    buffer[n] = '\0';
                    printf("Received from fd %d: %s\n", client_fd, buffer);
                }
            }
        }
    }

    close(listen_fd);
    close(epoll_fd);
    return 0;
}