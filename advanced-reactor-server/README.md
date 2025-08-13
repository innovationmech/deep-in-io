# Advanced Reactor Server

A high-performance multi-threaded reactor server implementation using epoll on Linux. This server demonstrates advanced I/O multiplexing patterns with separate thread pools for I/O operations and business logic processing.

## Architecture

The server uses a **Reactor pattern** with the following components:

- **Main Thread**: Accepts new connections using epoll
- **I/O Thread Pool**: Handles client I/O operations (read/write) 
- **Worker Thread Pool**: Processes business logic and HTTP requests
- **Task Queue**: Thread-safe queue for task distribution
- **Epoll Wrapper**: Abstraction layer for epoll operations

## Features

- **High Concurrency**: Multi-threaded architecture with separate I/O and worker threads
- **Efficient I/O**: Uses epoll for scalable event-driven I/O multiplexing
- **Thread Pool**: Configurable thread pools to optimize resource usage
- **Non-blocking I/O**: All socket operations are non-blocking
- **HTTP Support**: Basic HTTP request/response handling
- **Performance Monitoring**: Built-in connection statistics
- **Memory Safe**: Proper resource management and cleanup

## Build and Run

### Prerequisites

- Linux operating system
- GCC compiler
- pthread library
- `wrk` or `ab` for load testing (optional)

### Building

```bash
# Build the server
make

# Or build and run
make run
```

### Usage

```bash
# Basic usage (default: port 8080, 4 I/O threads, 8 worker threads)
./reactor_server

# Custom configuration
./reactor_server -p 8080 -i 4 -w 8

# Show help
./reactor_server --help
```

#### Command Line Options

- `-p, --port PORT`: Server port (default: 8080)
- `-i, --io-threads NUM`: Number of I/O threads (default: 4, max: 16)
- `-w, --worker-threads NUM`: Number of worker threads (default: 8, max: 32)
- `-h, --help`: Show help message

## Testing

### Load Testing

```bash
# Run performance test with wrk
make test

# Manual testing with wrk
wrk -t12 -c400 -d30s http://localhost:8080/
```

### Client Testing

```bash
# Build and run test client
gcc test_client.c -o test_client -pthread
./test_client
```

The test client creates multiple threads that send concurrent HTTP requests to measure server performance.

## Development

### Debug Build

```bash
# Debug with GDB
make debug

# Or run with debugging symbols
gcc -Wall -Wextra -O0 -g -pthread *.c -o reactor_server_debug
gdb ./reactor_server_debug
```

### Performance Profiling

```bash
# Profile with built-in script
./perf.sh

# Monitor with custom script
./monitor.sh
```

### Memory Checking

```bash
# Run with Valgrind
./valgrind.sh
```

## File Structure

```
advanced-reactor-server/
├── main.c              # Entry point and argument parsing
├── server.c/h          # Main server logic and reactor implementation
├── io_thread.c/h       # I/O thread pool implementation
├── thread_pool.c/h     # Worker thread pool implementation
├── task_queue.c/h      # Thread-safe task queue
├── epoll_wrapper.c/h   # Epoll abstraction layer
├── common.h            # Common definitions and structures
├── test_client.c       # Multi-threaded test client
├── Makefile            # Build configuration
├── build.sh            # Build script
├── run_test.sh         # Test runner script
├── monitor.sh          # System monitoring script
├── perf.sh             # Performance profiling script
├── valgrind.sh         # Memory leak detection script
└── debug.gdb           # GDB debugging commands
```

## Performance Characteristics

- **Concurrency**: Supports thousands of concurrent connections
- **Throughput**: High request/second capacity with proper thread tuning
- **Latency**: Low latency due to non-blocking I/O and efficient task distribution
- **Scalability**: Scales well with CPU cores through configurable thread pools

## Implementation Details

### Thread Model

1. **Main Thread**: Runs the main epoll loop, accepts connections, and distributes them to I/O threads
2. **I/O Threads**: Each has its own epoll instance to handle client socket I/O
3. **Worker Threads**: Process HTTP requests and generate responses

### Connection Lifecycle

1. Main thread accepts connection and assigns it to an I/O thread
2. I/O thread monitors the connection for read/write events
3. When data arrives, I/O thread queues a processing task for worker threads
4. Worker thread processes the request and queues a write task back to I/O thread
5. I/O thread sends response and continues monitoring or closes connection

### Memory Management

- Connections are managed with proper lifecycle handling
- Thread-safe reference counting prevents memory leaks
- Graceful cleanup on server shutdown
