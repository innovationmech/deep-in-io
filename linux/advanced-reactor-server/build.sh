#!/bin/bash
# build.sh - 编译脚本

echo "Building Reactor Server..."

# 创建构建目录
mkdir -p build
cd build

# 编译各个模块
echo "Compiling modules..."
gcc -c ../epoll_wrapper.c -o epoll_wrapper.o -Wall -Wextra -O2 -g -pthread
gcc -c ../task_queue.c -o task_queue.o -Wall -Wextra -O2 -g -pthread
gcc -c ../thread_pool.c -o thread_pool.o -Wall -Wextra -O2 -g -pthread
gcc -c ../io_thread.c -o io_thread.o -Wall -Wextra -O2 -g -pthread
gcc -c ../server.c -o server.o -Wall -Wextra -O2 -g -pthread
gcc -c ../main.c -o main.o -Wall -Wextra -O2 -g -pthread

# 链接
echo "Linking..."
gcc *.o -o reactor_server -pthread

if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo "Run with: ./build/reactor_server"
else
    echo "Build failed!"
    exit 1
fi