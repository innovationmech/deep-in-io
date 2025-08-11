# Deep-in-IO Epoll Server Examples

This is a demonstration project showcasing Linux epoll I/O multiplexing technology with server examples.

## Project Contents

This project contains two epoll server implementations:

- **basic_epoll_server.c** - Basic single-threaded epoll server
- **epoll_multithread_server.c** - Multi-threaded epoll server

## Quick Start

### System Requirements

- Linux operating system (epoll is Linux-specific)
- GCC compiler
- Make build tool

### Compilation

```bash
# Compile all servers
make all

# Or compile individually
make basic_epoll_server
make epoll_mt_server
```

### Running

```bash
# Run basic server
make run-basic
# or
./basic_epoll_server

# Run multi-threaded server
make run-mt
# or
./epoll_mt_server
```

The server will listen for connections on port 8888.

### Testing Connection

Use telnet or other client tools to connect to the server:

```bash
telnet localhost 8888
```

## Non-Linux Platform Support

If you're using macOS or Windows, you can run through Docker:

```bash
# Build Docker image
make docker-build

# Run basic server
make docker-run-basic

# Run multi-threaded server
make docker-run-mt
```

## Other Commands

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