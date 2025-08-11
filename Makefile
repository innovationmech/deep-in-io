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

# Install (optional)
install: $(TARGETS)
	cp $(TARGETS) /usr/local/bin/

# Uninstall (optional)
uninstall:
	rm -f /usr/local/bin/basic_epoll_server /usr/local/bin/epoll_mt_server

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
	@echo "  install      - Install binaries to /usr/local/bin"
	@echo "  uninstall    - Remove installed binaries"
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

# Docker support (for non-Linux platforms)
docker-build:
	@echo "Building Docker image for epoll servers..."
	docker build -t epoll-servers .

docker-run-basic:
	@echo "Running basic epoll server in Docker..."
	docker run -p 8888:8888 epoll-servers ./basic_epoll_server

docker-run-mt:
	@echo "Running multithreaded epoll server in Docker..."
	docker run -p 8888:8888 epoll-servers ./epoll_mt_server

# Declare phony targets
.PHONY: all clean rebuild run-basic run-mt install uninstall help docker-build docker-run-basic docker-run-mt