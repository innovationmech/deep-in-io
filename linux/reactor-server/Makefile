# Makefile for reactor-server

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O0 -g
LDFLAGS = -lpthread

TARGET = reactor-server
SOURCES = main.c util.c thread_pool.c io_thread.c conn_map.c global_queue.c
HEADERS = util.h thread_pool.h io_thread.h conn_map.h global_queue.h

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)