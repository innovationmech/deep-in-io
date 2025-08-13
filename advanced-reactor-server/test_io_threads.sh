#!/bin/bash

echo "=== IO线程数量性能测试 ==="

for io_threads in 2 4 6 8 12 16; do
    echo ""
    echo "🧪 测试 $io_threads 个IO线程..."
    
    # 启动服务器
    ./reactor_server -i $io_threads > server.log 2>&1 &
    server_pid=$!
    
    # 等待服务器启动
    sleep 1
    
    # 运行测试
    result=$(./test_client | grep "Success rate\|Requests per second")
    
    echo "IO线程数: $io_threads"
    echo "$result"
    
    # 关闭服务器
    kill $server_pid 2>/dev/null
    wait $server_pid 2>/dev/null
    
    sleep 1
done

echo ""
echo "=== 测试完成 ==="