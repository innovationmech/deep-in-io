# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a demonstration project showcasing high-performance I/O multiplexing server implementations with cross-platform support. The codebase contains multiple server implementations using different patterns and technologies:

- **Cross-platform Advanced Reactor Server**: Full-featured multi-threaded server supporting both Linux (epoll) and macOS (kqueue)
- **Linux-specific servers**: Using epoll for efficient event-driven I/O
- **macOS-specific servers**: Using kqueue for BSD-based systems
- **Educational examples**: Simple to advanced reactor pattern implementations

## Build Commands

### Linux Epoll Servers
```bash
# Build all epoll servers
cd linux/epoll && make all

# Build specific servers
make basic_epoll_server    # Single-threaded epoll server
make epoll_mt_server       # Multi-threaded epoll server

# Run servers
make run-basic             # Run basic server on port 8888
make run-mt                # Run multi-threaded server on port 8888
```

### Advanced Reactor Server (Cross-Platform)
```bash
cd advanced-reactor-server

# Configure and build
./configure                # Auto-detect platform and debugger
make                       # Build reactor_server (auto-cleans .o files)
make run                   # Build and run (port 8080)
./reactor_server -p 8080 -i 4 -w 8  # Custom config

# Cross-platform testing
make test                  # Run wrk/curl performance test
make test-client           # Run with custom test client
./run_test.sh             # Run comprehensive tests

# Cross-platform debugging
make debug                 # Auto-detect debugger (GDB/LLDB)
make debug-gdb             # Force GDB (Linux)
make debug-lldb            # Force LLDB (macOS)

# Development tools
make memcheck              # Memory leak detection (valgrind if available)
./perf.sh                 # Performance profiling (Linux)
./monitor.sh              # System monitoring
```

### Reactor Server (Simplified)
```bash
cd linux/reactor-server

# Build and run
make clean && make        # Clean build
./reactor-server          # Run on port 8888
```

### macOS Kqueue Server
```bash
cd osx/kqueue

# Build and run
make all                  # Build kqueue_server
make run                  # Run on port 8888
```

## Architecture Patterns

### Advanced Reactor Server Architecture

The cross-platform advanced reactor server uses a sophisticated multi-threaded pattern:

1. **Main Thread**: Accepts connections and distributes them to I/O threads
2. **I/O Thread Pool**: Each thread has its own event loop instance (epoll/kqueue) for handling socket I/O
3. **Worker Thread Pool**: Processes business logic and HTTP requests
4. **Task Queue**: Thread-safe queue for inter-thread communication
5. **Connection Management**: Thread-safe connection lifecycle with reference counting
6. **Event Loop Abstraction**: Unified API supporting both epoll (Linux) and kqueue (macOS)

### Key Design Principles

- **Cross-platform compatibility**: Single codebase supporting Linux and macOS
- **Non-blocking I/O**: All socket operations are non-blocking
- **Edge-triggered mode**: For maximum performance (EPOLLET/EV_CLEAR)
- **Thread separation**: I/O threads handle network events, worker threads handle business logic
- **Load balancing**: Round-robin distribution of connections across I/O threads
- **Memory safety**: Proper resource management with mutex locks and reference counting
- **Platform optimization**: Uses the best I/O multiplexing mechanism for each platform

## Testing Commands

```bash
# Load testing with wrk
wrk -t12 -c400 -d30s http://localhost:8080/

# Test with multiple clients
gcc test_client.c -o test_client -pthread && ./test_client

# Connect with telnet
telnet localhost 8888

# Test different thread configurations
./test_thread_combinations.sh
./test_io_threads.sh
```

## Common Development Tasks

```bash
# Clean all builds
make clean

# Rebuild everything
make rebuild

# Check for memory leaks
valgrind --leak-check=full ./reactor_server

# Profile performance
perf record -g ./reactor_server
perf report

# Monitor system resources during runtime
./monitor.sh
```

## Important Files and Their Roles

### Advanced Reactor Server Components (advanced-reactor-server/)
- `server.c/h`: Main server logic and reactor implementation
- `io_thread.c/h`: I/O thread pool handling platform-specific events
- `thread_pool.c/h`: Worker thread pool for request processing
- `task_queue.c/h`: Thread-safe task queue implementation
- `event_loop.c/h`: Cross-platform event loop abstraction
- `event_loop_epoll.c`: Linux epoll backend implementation
- `event_loop_kqueue.c`: macOS kqueue backend implementation
- `connection.c`: Thread-safe connection lifecycle management
- `configure`: Platform detection and configuration script
- `debug.gdb/debug.lldb`: Platform-specific debugging configurations

### Configuration Headers
- `common.h`: Common definitions, structures, and includes
- Thread pool sizes: MAX_IO_THREADS, MAX_WORKER_THREADS
- Buffer sizes: BUFFER_SIZE (4096), MAX_EVENTS (2048)
- Connection limits: BACKLOG (1024)

## Performance Tuning

- **I/O Threads**: Default 4, optimal is usually number of CPU cores
- **Worker Threads**: Default 8, increase for CPU-intensive workloads
- **Edge-triggered mode**: Requires handling EAGAIN properly
- **TCP_NODELAY**: Enabled to reduce latency
- **SO_REUSEADDR**: Enabled for quick server restart