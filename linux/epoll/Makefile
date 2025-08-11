# Makefile for epoll server examples

# Compiler and compilation options
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
LDFLAGS = 

# Detect operating system
UNAME_S := $(shell uname -s)

# Target files (adjusted based on operating system)
ifeq ($(UNAME_S),Linux)
    TARGETS = basic_epoll_server epoll_mt_server
else
    TARGETS = 
    $(warning Warning: epoll is Linux-specific. These programs require Linux to compile and run.)
endif

# Default target
all: $(TARGETS)

# Basic epoll server (single-threaded)
basic_epoll_server: basic_epoll_server.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Multi-threaded epoll server
epoll_mt_server: epoll_multithread_server.c
	$(CC) $(CFLAGS) -o $@ $< -lpthread $(LDFLAGS)

# Clean target files
clean:
	rm -f $(TARGETS)

# Rebuild
rebuild: clean all

# Run basic server
run-basic: basic_epoll_server
	./basic_epoll_server

# Run multi-threaded server
run-mt: epoll_mt_server
	./epoll_mt_server


# Help information
help:
	@echo "Available targets:"
ifeq ($(UNAME_S),Linux)
	@echo "  all          - Build all targets"
	@echo "  basic_epoll_server - Build basic epoll server"
	@echo "  epoll_mt_server    - Build multithreaded epoll server"
	@echo "  clean        - Remove all built files"
	@echo "  rebuild      - Clean and build all"
	@echo "  run-basic    - Build and run basic server"
	@echo "  run-mt       - Build and run multithreaded server"
else
	@echo "  Platform: $(UNAME_S) (not supported)"
	@echo "  Note: epoll is Linux-specific. To compile and run these programs:"
	@echo "    1. Use a Linux system or VM"
	@echo "    2. Use Docker with Linux container"
	@echo "    3. Use WSL (Windows Subsystem for Linux)"
	@echo "  "
	@echo "  Available targets on this platform:"
	@echo "  clean        - Remove any existing built files"
	@echo "  docker-build - Build Docker image for epoll servers"
	@echo "  docker-run-basic - Run basic server in Docker container"
	@echo "  docker-run-mt    - Run multithreaded server in Docker container"
endif
	@echo "  help         - Show this help message"


# Declare phony targets
.PHONY: all clean rebuild run-basic run-mt help