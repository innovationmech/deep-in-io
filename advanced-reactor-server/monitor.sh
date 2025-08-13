#!/bin/bash
# monitor.sh - 服务器监控脚本

SERVER_PID=$(pgrep reactor_server)

if [ -z "$SERVER_PID" ]; then
    echo "Server is not running!"
    exit 1
fi

echo "Monitoring Reactor Server (PID: $SERVER_PID)"
echo "Press Ctrl+C to stop monitoring"
echo ""

while true; do
    clear
    echo "========== Reactor Server Monitor =========="
    echo "Time: $(date)"
    echo ""
    
    # CPU 和内存使用
    echo "=== Resource Usage ==="
    ps -p $SERVER_PID -o pid,ppid,cmd,%cpu,%mem,rss,vsz
    echo ""
    
    # 线程信息
    echo "=== Thread Information ==="
    ps -T -p $SERVER_PID | head -20
    echo "Total threads: $(ps -T -p $SERVER_PID | wc -l)"
    echo ""
    
    # 网络连接统计
    echo "=== Network Connections ==="
    echo "ESTABLISHED connections: $(netstat -anp 2>/dev/null | grep $SERVER_PID | grep ESTABLISHED | wc -l)"
    echo "TIME_WAIT connections: $(netstat -an | grep :8080 | grep TIME_WAIT | wc -l)"
    echo "LISTEN sockets: $(netstat -anp 2>/dev/null | grep $SERVER_PID | grep LISTEN | wc -l)"
    echo ""
    
    # 文件描述符
    echo "=== File Descriptors ==="
    if [ -d "/proc/$SERVER_PID/fd" ]; then
        echo "Open FDs: $(ls /proc/$SERVER_PID/fd | wc -l)"
        echo "Limit: $(cat /proc/$SERVER_PID/limits | grep 'open files' | awk '{print $4}')"
    fi
    echo ""
    
    # 系统负载
    echo "=== System Load ==="
    uptime
    echo ""
    
    # 网络流量（需要 root 权限）
    if [ "$EUID" -eq 0 ]; then
        echo "=== Network Traffic (last 5 seconds) ==="
        timeout 5 tcpdump -i lo -p -n -c 100 port 8080 2>/dev/null | \
            awk '{print $3, $5}' | grep -E "^[0-9]" | wc -l | \
            awk '{print "Packets captured: " $1}'
    fi
    
    sleep 5
done