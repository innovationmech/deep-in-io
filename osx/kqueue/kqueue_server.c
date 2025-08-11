#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

#define PORT 8888
#define MAX_EVENTS 10
#define BUFFER_SIZE 1024

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    int listen_fd, conn_fd, kq;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    // create socket
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    set_nonblocking(listen_fd);

    // set address reuse
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // bind address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    // listen
    listen(listen_fd, 10);

    // create kqueue
    kq = kqueue();

    // register event
    struct kevent change_event;
    EV_SET(&change_event, listen_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
    kevent(kq, &change_event, 1, NULL, 0, NULL);

    printf("kqueue server running on port %d...\n", PORT);

    // event loop
    while (1) {
        struct kevent events[MAX_EVENTS];
        int nev = kevent(kq, NULL, 0, events, MAX_EVENTS, NULL);
        if (nev == -1) {
            perror("kevent");
            break;
        }

        for (int i = 0; i < nev; ++i) {
            int fd = (int)(intptr_t)events[i].ident;

            if (fd == listen_fd) {
                // accept connection
                conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
                set_nonblocking(conn_fd);
                printf("New connection accepted: fd=%d\n", conn_fd);

                // register read event
                struct kevent new_event;
                EV_SET(&new_event, conn_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
                kevent(kq, &new_event, 1, NULL, 0, NULL);
            } else if (events[i].filter == EVFILT_READ) {
                // read event
                int n = read(fd, buffer, BUFFER_SIZE);
                if (n <= 0) {
                    printf("Client disconnected: fd=%d\n", fd);
                    close(fd);
                    // unregister event
                    struct kevent del_event;
                    EV_SET(&del_event, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
                    kevent(kq, &del_event, 1, NULL, 0, NULL);
                } else {
                    buffer[n] = '\0';
                    printf("Received from fd %d: %s\n", fd, buffer);
                }
            }
        }
    }

    close(listen_fd);
    close(kq);
    return 0;
}