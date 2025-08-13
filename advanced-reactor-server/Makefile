# Cross-platform Makefile
# Include configuration if exists
-include Makefile.config

# Default values if not configured
CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -g -pthread
LDFLAGS ?= -pthread

# Detect platform if not configured
ifeq ($(PLATFORM),)
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        PLATFORM = Linux
        PLATFORM_SRCS = event_loop_epoll.c
        CFLAGS += -D__linux__ -DUSE_EPOLL
    else ifeq ($(UNAME_S),Darwin)
        PLATFORM = Darwin
        PLATFORM_SRCS = event_loop_kqueue.c
        CFLAGS += -D__APPLE__ -DUSE_KQUEUE
    else
        $(error Unsupported platform: $(UNAME_S))
    endif
endif

# Add config.h support
ifneq ($(wildcard config.h),)
    CFLAGS += -DHAVE_CONFIG_H
endif

# Common source files
COMMON_SRCS = main.c \
              server.c \
              io_thread.c \
              thread_pool.c \
              task_queue.c \
              connection.c \
              event_loop.c

# All source files
SRCS = $(COMMON_SRCS) $(PLATFORM_SRCS)

# Object files
OBJS = $(SRCS:.c=.o)

# Executables
TARGET = reactor_server
TEST_CLIENT = test_client

# Default target
all: $(TARGET)

# Build all targets including test client
all-tests: $(TARGET) $(TEST_CLIENT)

# Configure before build
configure:
	@./configure

# Link main executable and clean intermediate files
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@rm -f $(OBJS)
	@echo "Successfully built $(TARGET) for $(PLATFORM)"

# Build test client
$(TEST_CLIENT): test_client.c
	$(CC) $(CFLAGS) test_client.c -o $(TEST_CLIENT) $(LDFLAGS)
	@echo "Successfully built $(TEST_CLIENT)"

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJS) $(TARGET) $(TEST_CLIENT) config.h Makefile.config
	@echo "Cleaned build files"

# Deep clean (including logs)
distclean: clean
	rm -f *.log

# Rebuild everything
rebuild: clean all

# Run the server
run: $(TARGET)
	./$(TARGET) -p 8080 -i 4 -w 8

# Run with custom settings
run-custom: $(TARGET)
	@echo "Usage: make run-custom ARGS='-p 8080 -i 4 -w 8'"
	./$(TARGET) $(ARGS)

# Cross-platform debugging
debug: $(TARGET)
	@if command -v lldb &> /dev/null; then \
		echo "Starting LLDB debugger..."; \
		lldb -s debug.lldb ./$(TARGET); \
	elif command -v gdb &> /dev/null; then \
		echo "Starting GDB debugger..."; \
		gdb -x debug.gdb ./$(TARGET); \
	else \
		echo "No debugger found. Please install gdb or lldb."; \
		exit 1; \
	fi

# Force GDB debugging (Linux)
debug-gdb: $(TARGET)
	@if command -v gdb &> /dev/null; then \
		gdb -x debug.gdb ./$(TARGET); \
	else \
		echo "GDB not found. Please install gdb."; \
		exit 1; \
	fi

# Force LLDB debugging (macOS)
debug-lldb: $(TARGET)
	@if command -v lldb &> /dev/null; then \
		lldb -s debug.lldb ./$(TARGET); \
	else \
		echo "LLDB not found. Please install lldb."; \
		exit 1; \
	fi

# Memory check with valgrind (if available)
memcheck: $(TARGET)
	@if command -v valgrind &> /dev/null; then \
		valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET); \
	else \
		echo "Valgrind not installed. Install it for memory checking."; \
	fi

# Performance test with wrk (if available)
test: $(TARGET)
	@echo "Starting server in background..."
	@./$(TARGET) -p 8080 -i 4 -w 8 &
	@SERVER_PID=$$!; \
	sleep 2; \
	if command -v wrk &> /dev/null; then \
		echo "Running performance test..."; \
		wrk -t12 -c400 -d30s http://localhost:8080/; \
	else \
		echo "wrk not installed. Using curl for basic test..."; \
		curl -s http://localhost:8080/ || true; \
	fi; \
	kill $$SERVER_PID 2>/dev/null || true

# Test with test client
test-client: $(TARGET) $(TEST_CLIENT)
	@echo "Starting server in background..."
	@./$(TARGET) -p 8080 -i 4 -w 8 > server_test.log 2>&1 &
	@SERVER_PID=$$!; \
	sleep 1; \
	echo "Running test client..."; \
	./$(TEST_CLIENT); \
	sleep 1; \
	kill $$SERVER_PID 2>/dev/null || true; \
	wait $$SERVER_PID 2>/dev/null || true; \
	echo "Server log:"; \
	tail -10 server_test.log

# Show help
help:
	@echo "Available targets:"
	@echo "  all        - Build the reactor server (default)"
	@echo "  all-tests  - Build reactor server and test client"
	@echo "  test_client- Build test client only"
	@echo "  configure  - Run the configure script"
	@echo "  clean      - Remove build files"
	@echo "  rebuild    - Clean and rebuild"
	@echo "  run        - Run the server with defaults"
	@echo "  debug      - Debug with platform-appropriate debugger"
	@echo "  debug-gdb  - Debug with GDB (Linux)"
	@echo "  debug-lldb - Debug with LLDB (macOS)"
	@echo "  memcheck   - Check memory leaks with valgrind"
	@echo "  test       - Run performance tests with wrk/curl"
	@echo "  test-client- Run test with custom test client"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Platform: $(PLATFORM)"
	@echo "Compiler: $(CC)"

.PHONY: all all-tests configure clean distclean rebuild run run-custom debug debug-gdb debug-lldb memcheck test test-client help