#!/bin/bash

echo "Starting Reactor Server..."
./build/reactor_server -p 8080 -i 4 -w 8 &
SERVER_PID=$!

sleep 2

echo "Running test client..."
gcc test_client.c -o test_client -pthread
./test_client

echo "Stopping server..."
kill $SERVER_PID

echo "Test completed!"