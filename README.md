# Deep-in-IO Server Examples

This is a demonstration project showcasing I/O multiplexing technology with cross-platform server examples.

## Project Contents

This project contains server implementations for different platforms:

### Linux (epoll)
- **basic_epoll_server.c** - Basic single-threaded epoll server
- **epoll_multithread_server.c** - Multi-threaded epoll server

### macOS (kqueue)
- **kqueue_server.c** - kqueue-based server for macOS

## Quick Start

### System Requirements

#### For Linux (epoll)
- Linux operating system
- GCC compiler
- Make build tool

#### For macOS (kqueue)
- macOS operating system
- Clang compiler (included with Xcode)
- Make build tool

### Compilation

#### Linux (epoll servers)
```bash
cd linux/epoll

# Compile all servers
make all

# Or compile individually
make basic_epoll_server
make epoll_mt_server
```

#### macOS (kqueue server)
```bash
cd osx/kqueue

# Compile kqueue server
make all
# or
make kqueue_server
```

### Running

#### Linux (epoll servers)
```bash
cd linux/epoll

# Run basic server
make run-basic
# or
./basic_epoll_server

# Run multi-threaded server
make run-mt
# or
./epoll_mt_server
```

#### macOS (kqueue server)
```bash
cd osx/kqueue

# Run kqueue server
make run
# or
./kqueue_server
```

All servers will listen for connections on port 8888.

### Testing Connection

Use telnet or other client tools to connect to the server:

```bash
telnet localhost 8888
```

## Cross-Platform Support

- **Linux**: Use epoll-based servers in `linux/epoll/`
- **macOS**: Use kqueue-based server in `osx/kqueue/`

## Other Commands

Each platform directory has its own Makefile with common commands:

```bash
# Clean compiled files
make clean

# Rebuild
make rebuild

# Show help
make help
```

## License

This project is for learning and demonstration purposes only.