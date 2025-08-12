// util.c
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void die(const char* msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}