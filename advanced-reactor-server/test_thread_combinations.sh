#!/bin/bash

echo "=== IO线程 + Worker线程组合性能测试 ==="

# 测试不同的线程组合
combinations=(
    "4 8"    # 4 IO, 8 Worker
    "4 16"   # 4 IO, 16 Worker  
    "6 12"   # 6 IO, 12 Worker
    "8 16"   # 8 IO, 16 Worker (当前默认)
    "8 24"   # 8 IO, 24 Worker
    "12 16"  # 12 IO, 16 Worker
    "12 24"  # 12 IO, 24 Worker
)

for combo in "${combinations[@]}"; do
    io_threads=$(echo $combo | cut -d' ' -f1)
    worker_threads=$(echo $combo | cut -d' ' -f2)
    
    echo ""
    echo "🧪 测试 IO:$io_threads, Worker:$worker_threads"
    
    # 启动服务器
    ./reactor_server -i $io_threads -w $worker_threads > server.log 2>&1 &
    server_pid=$!
    
    # 等待服务器启动
    sleep 1
    
    # 运行测试
    result=$(./test_client | grep -E "Success rate|Requests per second|Total test time")
    
    echo "配置: IO=$io_threads, Worker=$worker_threads"
    echo "$result"
    echo "---"
    
    # 关闭服务器
    kill $server_pid 2>/dev/null
    wait $server_pid 2>/dev/null
    
    sleep 1
done

echo ""
echo "=== 测试完成，寻找最佳配置组合 ==="