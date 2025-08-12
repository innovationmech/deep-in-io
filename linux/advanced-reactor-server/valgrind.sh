#!/bin/bash

echo "Running Valgrind memory check..."

valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         --log-file=valgrind.log \
         ./build/reactor_server -p 8080 -i 2 -w 4 &

VALGRIND_PID=$!

sleep 5

# 运行一些测试
for i in {1..100}; do
    curl -s http://localhost:8080/ > /dev/null &
done

wait

sleep 5

kill $VALGRIND_PID

echo "Memory check complete. Results in valgrind.log"
grep "ERROR SUMMARY" valgrind.log