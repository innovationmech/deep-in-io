// main.c
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "server.h"

static void print_usage(const char *prog) {
    printf("Usage: %s [options]\n", prog);
    printf("Options:\n");
    printf("  -p, --port PORT          Server port (default: 8080)\n");
    printf("  -i, --io-threads NUM     Number of IO threads (default: 4)\n");
    printf("  -w, --worker-threads NUM Number of worker threads (default: 8)\n");
    printf("  -h, --help               Show this help message\n");
}

int main(int argc, char *argv[]) {
    int port = 8080;
    int io_threads = 12;  // 基于测试结果的最优配置
    int worker_threads = 24;
    
    // 解析命令行参数
    static struct option long_options[] = {
        {"port", required_argument, 0, 'p'},
        {"io-threads", required_argument, 0, 'i'},
        {"worker-threads", required_argument, 0, 'w'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int opt;
    while ((opt = getopt_long(argc, argv, "p:i:w:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                break;
            case 'i':
                io_threads = atoi(optarg);
                if (io_threads <= 0 || io_threads > MAX_THREADS) {
                    fprintf(stderr, "Invalid number of IO threads: %d\n", io_threads);
                    exit(EXIT_FAILURE);
                }
                break;
            case 'w':
                worker_threads = atoi(optarg);
                if (worker_threads <= 0 || worker_threads > MAX_THREADS * 2) {
                    fprintf(stderr, "Invalid number of worker threads: %d\n", worker_threads);
                    exit(EXIT_FAILURE);
                }
                break;
            case 'h':
                print_usage(argv[0]);
                exit(EXIT_SUCCESS);
            default:
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    
    printf("========================================\n");
    printf("Reactor Server Configuration:\n");
    printf("  Port: %d\n", port);
    printf("  IO Threads: %d\n", io_threads);
    printf("  Worker Threads: %d\n", worker_threads);
    printf("========================================\n\n");
    
    // 创建并启动服务器
    reactor_server_t *server = server_create(port, io_threads, worker_threads);
    if (!server) {
        fprintf(stderr, "Failed to create server\n");
        exit(EXIT_FAILURE);
    }
    
    // 运行服务器
    int ret = server_start(server);
    
    // 清理
    server_destroy(server);
    
    return ret;
}