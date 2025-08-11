# Makefile for kqueue server example

# Compiler and compilation options
CC = clang
CFLAGS = -Wall -Wextra -std=c99 -O2
LDFLAGS = 

# Detect operating system
UNAME_S := $(shell uname -s)

# Target files (adjusted based on operating system)
ifeq ($(UNAME_S),Darwin)
    TARGETS = kqueue_server
else
    TARGETS = 
    $(warning Warning: kqueue is macOS-specific. This program requires macOS to compile and run.)
endif

# Default target
all: $(TARGETS)

# kqueue server
kqueue_server: kqueue_server.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Clean target files
clean:
	rm -f $(TARGETS)

# Rebuild
rebuild: clean all

# Run kqueue server
run: kqueue_server
	./kqueue_server

# Help information
help:
	@echo "Available targets:"
ifeq ($(UNAME_S),Darwin)
	@echo "  all          - Build all targets"
	@echo "  kqueue_server - Build kqueue server"
	@echo "  clean        - Remove all built files"
	@echo "  rebuild      - Clean and build all"
	@echo "  run          - Build and run kqueue server"
else
	@echo "  Platform: $(UNAME_S) (not supported)"
	@echo "  Note: kqueue is macOS-specific. To compile and run this program:"
	@echo "    1. Use a macOS system"
	@echo "    2. Use a macOS virtual machine"
	@echo "  "
	@echo "  Available targets on this platform:"
	@echo "  clean        - Remove any existing built files"
endif
	@echo "  help         - Show this help message"

# Declare phony targets
.PHONY: all clean rebuild run help