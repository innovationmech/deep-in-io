# Reactor Server

A high-performance multi-threaded reactor-pattern server implementation in C using epoll for Linux systems.

## Features

- **Multi-threaded Reactor Pattern**: Separate I/O threads and worker threads for optimal performance
- **Edge-triggered Epoll**: Efficient event-driven I/O handling
- **Thread Pool**: Configurable worker thread pool for request processing
- **Connection Load Balancing**: Round-robin distribution of connections across I/O threads
- **Non-blocking I/O**: All socket operations are non-blocking for maximum concurrency

## Architecture

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│ Main Thread │────│ I/O Thread  │────│ Worker Pool │
│   (Accept)  │    │  (epoll)    │    │ (Process)   │
└─────────────┘    └─────────────┘    └─────────────┘
       │                   │                   │
       │            ┌─────────────┐    ┌─────────────┐
       └────────────│ I/O Thread  │    │ Global Queue│
                    │  (epoll)    │────│  (Tasks)    │
                    └─────────────┘    └─────────────┘
```

## Components

### Core Files
- `main.c` - Main server entry point and connection acceptance
- `io_thread.c/h` - I/O thread implementation with epoll event handling
- `thread_pool.c/h` - Worker thread pool for request processing
- `global_queue.c/h` - Thread-safe global task queue
- `conn_map.c/h` - Connection-to-buffer mapping for response handling
- `util.c/h` - Utility functions (non-blocking socket setup, error handling)

### Configuration
- `PORT`: Server listening port (default: 8888)
- `MAX_IO_THREADS`: Number of I/O threads (default: 4)
- `MAX_WORKER_THREADS`: Number of worker threads (default: 1)
- `MAX_TASKS`: Maximum tasks in global queue (default: 1024)

## Building and Running

### Prerequisites
- GCC compiler
- Linux system with epoll support
- pthread library

### Build
```bash
make clean
make
```

### Run
```bash
./reactor-server
```

The server will start listening on port 8888.

### Testing
```bash
# Connect using telnet
telnet localhost 8888

# Send messages
hello world
test message
```

## How It Works

1. **Main Thread**: Accepts new connections and distributes them to I/O threads using round-robin
2. **I/O Threads**: Handle socket events using epoll in edge-triggered mode
   - Read incoming data from client connections
   - Add received data to the global task queue
   - Handle response writing back to clients
3. **Worker Threads**: Process tasks from the global queue
   - Generate echo responses for received messages
   - Print received messages to console
4. **Global Queue**: Thread-safe task queue for communication between I/O and worker threads

## Key Design Decisions

- **Simplified Memory Management**: Uses static arrays instead of dynamic allocation to avoid memory leaks
- **Global Task Queue**: Replaces complex inter-thread communication with a simple global queue
- **Polling Instead of Condition Variables**: Uses polling to avoid pthread condition variable issues
- **Edge-triggered Epoll**: Maximizes performance by processing all available data per event

## Performance Characteristics

- Handles multiple concurrent connections efficiently
- Non-blocking I/O prevents thread blocking
- Load balancing across multiple I/O threads
- Configurable thread pool size based on workload

## Limitations

- Currently implements simple echo functionality
- Response writing is basic (no complex buffering)
- Fixed-size global task queue
- No graceful shutdown mechanism

## Future Enhancements

- Implement proper response queuing and writing
- Add configuration file support
- Add logging system
- Implement graceful shutdown
- Add connection timeouts and cleanup
- Performance monitoring and statistics
