# 高级反应器服务器

一个基于 Linux epoll 的高性能多线程反应器服务器实现。该服务器展示了高级 I/O 多路复用模式，使用独立的线程池分别处理 I/O 操作和业务逻辑。

## 架构设计

服务器采用**反应器模式**，包含以下组件：

- **主线程**：使用 epoll 接受新连接
- **I/O 线程池**：处理客户端 I/O 操作（读/写）
- **工作线程池**：处理业务逻辑和 HTTP 请求
- **任务队列**：线程安全的任务分发队列
- **Epoll 封装层**：epoll 操作的抽象层

## 功能特性

- **高并发**：多线程架构，I/O 和工作线程分离
- **高效 I/O**：使用 epoll 实现可扩展的事件驱动 I/O 多路复用
- **线程池**：可配置的线程池优化资源使用
- **非阻塞 I/O**：所有套接字操作均为非阻塞
- **HTTP 支持**：基本的 HTTP 请求/响应处理
- **性能监控**：内置连接统计功能
- **内存安全**：适当的资源管理和清理

## 编译和运行

### 前置要求

- Linux 操作系统
- GCC 编译器
- pthread 库
- `wrk` 或 `ab` 用于负载测试（可选）

### 编译

```bash
# 编译服务器
make

# 或者编译并运行
make run
```

### 使用方法

```bash
# 基本使用（默认：端口 8080，4个 I/O 线程，8个工作线程）
./reactor_server

# 自定义配置
./reactor_server -p 8080 -i 4 -w 8

# 显示帮助
./reactor_server --help
```

#### 命令行选项

- `-p, --port PORT`：服务器端口（默认：8080）
- `-i, --io-threads NUM`：I/O 线程数量（默认：4，最大：16）
- `-w, --worker-threads NUM`：工作线程数量（默认：8，最大：32）
- `-h, --help`：显示帮助信息

## 测试

### 负载测试

```bash
# 使用 wrk 运行性能测试
make test

# 手动使用 wrk 测试
wrk -t12 -c400 -d30s http://localhost:8080/
```

### 客户端测试

```bash
# 编译并运行测试客户端
gcc test_client.c -o test_client -pthread
./test_client
```

测试客户端会创建多个线程发送并发 HTTP 请求来测量服务器性能。

## 开发

### 调试编译

```bash
# 使用 GDB 调试
make debug

# 或者运行调试版本
gcc -Wall -Wextra -O0 -g -pthread *.c -o reactor_server_debug
gdb ./reactor_server_debug
```

### 性能分析

```bash
# 使用内置脚本进行性能分析
./perf.sh

# 使用自定义脚本监控
./monitor.sh
```

### 内存检查

```bash
# 使用 Valgrind 运行
./valgrind.sh
```

## 文件结构

```
advanced-reactor-server/
├── main.c              # 入口点和参数解析
├── server.c/h          # 主服务器逻辑和反应器实现
├── io_thread.c/h       # I/O 线程池实现
├── thread_pool.c/h     # 工作线程池实现
├── task_queue.c/h      # 线程安全任务队列
├── epoll_wrapper.c/h   # Epoll 抽象层
├── common.h            # 公共定义和结构体
├── test_client.c       # 多线程测试客户端
├── Makefile            # 编译配置
├── build.sh            # 编译脚本
├── run_test.sh         # 测试运行脚本
├── monitor.sh          # 系统监控脚本
├── perf.sh             # 性能分析脚本
├── valgrind.sh         # 内存泄漏检测脚本
└── debug.gdb           # GDB 调试命令
```

## 性能特性

- **并发性**：支持数千个并发连接
- **吞吐量**：通过适当的线程调优实现高请求/秒能力
- **延迟**：非阻塞 I/O 和高效任务分发带来的低延迟
- **可扩展性**：通过可配置线程池在多核 CPU 上良好扩展

## 实现细节

### 线程模型

1. **主线程**：运行主 epoll 循环，接受连接并分发到 I/O 线程
2. **I/O 线程**：每个都有自己的 epoll 实例来处理客户端套接字 I/O
3. **工作线程**：处理 HTTP 请求并生成响应

### 连接生命周期

1. 主线程接受连接并将其分配给 I/O 线程
2. I/O 线程监控连接的读/写事件
3. 当数据到达时，I/O 线程为工作线程队列处理任务
4. 工作线程处理请求并将写任务队列回 I/O 线程
5. I/O 线程发送响应并继续监控或关闭连接

### 内存管理

- 连接通过适当的生命周期处理进行管理
- 线程安全的引用计数防止内存泄漏
- 服务器关闭时优雅清理
