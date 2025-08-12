#!/bin/bash

echo "Starting performance profiling..."

# 记录性能数据
sudo perf record -g -p $(pgrep reactor_server) -o perf.data sleep 30

# 生成报告
sudo perf report -i perf.data