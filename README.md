# Deep-in-IO

A collection of high-performance I/O multiplexing server implementations demonstrating various architectural patterns and cross-platform techniques.

## Project Structure

```
deep-in-io/
├── advanced-reactor-server/   # Cross-platform reactor server (Linux & macOS)
├── linux/
│   ├── epoll/                 # Basic epoll examples (Linux-specific)
│   └── reactor-server/        # Simple reactor pattern (Linux-specific)
└── osx/
    └── kqueue/                # Simple kqueue example (macOS-specific)
```

## Featured Project

### 🚀 Advanced Reactor Server (Cross-Platform)

**Location**: `advanced-reactor-server/`

A production-ready, cross-platform reactor server that automatically adapts to the underlying platform:
- **Linux**: Uses epoll for high-performance I/O multiplexing
- **macOS**: Uses kqueue for optimal BSD performance
- **Features**: Multi-threaded reactor pattern, configurable thread pools, HTTP support, cross-platform debugging
- **Documentation**: See [README-CROSSPLATFORM.md](advanced-reactor-server/README-CROSSPLATFORM.md)

```bash
cd advanced-reactor-server
./configure && make
./reactor_server -p 8080 -i 4 -w 8
```

## Educational Examples

### Linux-Specific Implementations

- **linux/epoll/**: Fundamental epoll-based servers demonstrating single-threaded and multi-threaded approaches
- **linux/reactor-server/**: Simple reactor pattern implementation for learning purposes

### macOS-Specific Implementation

- **osx/kqueue/**: Basic BSD kqueue-based server for macOS systems

## Documentation

Each sub-project contains its own README with detailed setup instructions, architecture explanations, and usage examples. Please refer to the individual project directories for specific documentation.

## License

This project is for learning and demonstration purposes only.