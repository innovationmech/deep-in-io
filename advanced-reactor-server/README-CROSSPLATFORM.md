# Cross-Platform Reactor Server

This is a high-performance, cross-platform reactor server implementation that supports both Linux (epoll) and macOS (kqueue) through a unified event loop abstraction layer.

## Features

- **Cross-platform support**: Runs on Linux and macOS
- **High performance**: Uses epoll on Linux and kqueue on macOS
- **Multi-threaded architecture**: Separate I/O and worker thread pools
- **Edge-triggered I/O**: Maximum performance with non-blocking sockets
- **Thread-safe**: Proper synchronization and reference counting
- **Configurable**: Customizable thread pool sizes and port

## Architecture

```
┌─────────────────────────────────────────┐
│         Event Loop Abstraction          │
├──────────────┬──────────────────────────┤
│  Linux/epoll │      macOS/kqueue        │
└──────────────┴──────────────────────────┘
                      ↓
┌─────────────────────────────────────────┐
│          Main Thread (Accept)           │
└─────────────────────────────────────────┘
                      ↓
┌─────────────────────────────────────────┐
│        I/O Thread Pool (Events)         │
└─────────────────────────────────────────┘
                      ↓
┌─────────────────────────────────────────┐
│     Worker Thread Pool (Processing)     │
└─────────────────────────────────────────┘
```

## Building

### Quick Start

```bash
# Configure and build
./configure
make

# Or use the build script
./build.sh
```

### Manual Configuration

```bash
# Run configure to detect platform
./configure

# This generates:
# - config.h: Platform-specific definitions
# - Makefile.config: Build configuration

# Build the server
make

# Clean build
make clean

# Rebuild
make rebuild
```

### Platform-Specific Notes

#### Linux
- Uses epoll for I/O multiplexing
- Supports edge-triggered mode (EPOLLET)
- Requires Linux kernel 2.6.8+

#### macOS
- Uses kqueue for I/O multiplexing
- Supports edge-triggered mode (EV_CLEAR)
- Works on macOS 10.5+

## Running

### Basic Usage

```bash
# Run with defaults (port 8080, 4 I/O threads, 8 worker threads)
./reactor_server

# Custom configuration
./reactor_server -p 8888 -i 8 -w 16

# Using make
make run
```

### Command Line Options

```
-p <port>       Server port (default: 8080)
-i <threads>    Number of I/O threads (default: 4)
-w <threads>    Number of worker threads (default: 8)
```

## Testing

### Performance Testing

```bash
# Run built-in test
make test

# Manual testing with wrk
wrk -t12 -c400 -d30s http://localhost:8080/

# Manual testing with ab
ab -n 10000 -c 100 http://localhost:8080/
```

### Memory Testing

```bash
# Check for memory leaks (Linux with valgrind)
make memcheck

# Or manually
valgrind --leak-check=full ./reactor_server
```

### Debug Mode

```bash
# Debug with gdb
make debug

# Or manually
gdb ./reactor_server
```

## File Structure

```
.
├── event_loop.h           # Event loop abstraction interface
├── event_loop.c           # Event loop implementation selector
├── event_loop_epoll.c     # Linux epoll backend
├── event_loop_kqueue.c    # macOS kqueue backend
├── server.h/c             # Main server logic
├── io_thread.h/c          # I/O thread pool
├── thread_pool.h/c        # Worker thread pool
├── task_queue.h/c         # Thread-safe task queue
├── connection.c           # Connection management
├── common.h               # Common definitions
├── main.c                 # Entry point
├── configure              # Platform detection script
└── Makefile               # Cross-platform makefile
```

## API Changes for Cross-Platform Support

### Old (Linux-only)
```c
epoll_wrapper_t *epoll = epoll_wrapper_create(MAX_EVENTS);
epoll_wrapper_add(epoll, fd, EPOLLIN | EPOLLET, data);
epoll_wrapper_wait(epoll, timeout);
```

### New (Cross-platform)
```c
event_loop_t *loop = event_loop_create(MAX_EVENTS);
event_loop_add(loop, fd, EVENT_READ | EVENT_ET, data);
event_loop_wait(loop, events, max_events, timeout);
```

## Performance Tuning

### Linux
```bash
# Increase file descriptor limits
ulimit -n 65535

# TCP tuning
sudo sysctl -w net.core.somaxconn=1024
sudo sysctl -w net.ipv4.tcp_tw_reuse=1
```

### macOS
```bash
# Increase file descriptor limits
ulimit -n 10240

# Check current limits
sysctl kern.maxfiles
sysctl kern.maxfilesperproc
```

## Debugging

### Cross-Platform Debug Support

The server supports debugging on both Linux (GDB) and macOS (LLDB) with platform-specific configurations:

#### Automatic Debug Target
```bash
# Automatically detects and uses the appropriate debugger
make debug
```

#### Platform-Specific Targets
```bash
# Force GDB debugging (Linux)
make debug-gdb

# Force LLDB debugging (macOS)  
make debug-lldb
```

#### Debug Configurations

**Linux (GDB)**: Uses `debug.gdb` configuration
- Breakpoints at key functions
- Custom print commands for structures
- Pretty printing enabled

**macOS (LLDB)**: Uses `debug.lldb` configuration
- Equivalent breakpoints and commands
- Custom aliases for structure inspection
- Memory debugging helpers

#### Debug Commands Available

**Common Commands**:
- `print_conn <addr>` - Print connection structure
- `print_server <addr>` - Print server structure
- `pstats` - Print server statistics
- `pworker` - Print worker task count

**LLDB-specific**:
- `hexdump <addr>` - Hex dump memory
- `memdump <addr>` - Memory dump (4-byte words)

#### Usage Examples
```bash
# Start debugging with arguments
(gdb) run -p 8080 -i 4 -w 8
(lldb) run -p 8080 -i 4 -w 8

# Set additional breakpoints
(gdb) break accept_connections
(lldb) breakpoint set --name accept_connections

# Print server statistics during runtime
(gdb) print_stats
(lldb) pstats
```

## Troubleshooting

### Build Issues

1. **Configure fails**: Ensure you have gcc/clang installed
2. **Make fails**: Check that all source files are present
3. **Platform not detected**: Manually set in Makefile
4. **Debugger not found**: Install gdb (Linux) or ensure lldb is available (macOS)

### Runtime Issues

1. **Port already in use**: Change port with `-p` option
2. **Too many open files**: Increase ulimit
3. **Connection refused**: Check firewall settings

### Debug Issues

1. **GDB not available on macOS**: Use `make debug-lldb` instead
2. **LLDB not available on Linux**: Use `make debug-gdb` instead  
3. **Breakpoints not hit**: Ensure debug symbols with `-g` flag
4. **Functions not found**: Some static functions may not be visible

## Contributing

When adding platform support:

1. Create `event_loop_<platform>.c` implementation
2. Update `configure` script for platform detection
3. Add platform-specific flags to Makefile
4. Test thoroughly on target platform

## License

See LICENSE file in the project root.

## Benchmarks

Typical performance on modern hardware:

- **Connections**: 10,000+ concurrent
- **Throughput**: 100,000+ req/sec
- **Latency**: < 1ms p99

Results vary based on hardware and configuration.