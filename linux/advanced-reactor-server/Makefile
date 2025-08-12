CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -pthread
LDFLAGS = -pthread

# 源文件
SRCS = main.c \
       server.c \
       io_thread.c \
       thread_pool.c \
       task_queue.c \
       epoll_wrapper.c

# 目标文件
OBJS = $(SRCS:.c=.o)

# 可执行文件
TARGET = reactor_server

# 默认目标
all: $(TARGET)

# 链接
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@rm -f $(OBJS)

# 编译
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 清理
clean:
	rm -f $(OBJS) $(TARGET)

# 运行
run: $(TARGET)
	./$(TARGET)

# 调试运行
debug: $(TARGET)
	gdb ./$(TARGET)

# 性能测试（使用 wrk 或 ab）
test: $(TARGET)
	@echo "Starting server in background..."
	./$(TARGET) &
	@sleep 2
	@echo "Running performance test..."
	wrk -t12 -c400 -d30s http://localhost:8080/
	@killall $(TARGET)

.PHONY: all clean run debug test
