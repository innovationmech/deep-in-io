#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define NUM_THREADS 10
#define REQUESTS_PER_THREAD 100

typedef struct {
    int thread_id;
    int successful_requests;
    int failed_requests;
    double total_time;
} thread_data_t;

void* client_thread(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    
    for (int i = 0; i < REQUESTS_PER_THREAD; i++) {
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        // 创建套接字
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            data->failed_requests++;
            continue;
        }
        
        // 连接服务器
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SERVER_PORT);
        inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
        
        if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            data->failed_requests++;
            close(sock);
            continue;
        }
        
        // 发送 HTTP 请求
        const char* request = "GET / HTTP/1.1\r\n"
                             "Host: localhost\r\n"
                             "Connection: keep-alive\r\n"
                             "\r\n";
        
        if (send(sock, request, strlen(request), 0) < 0) {
            data->failed_requests++;
            close(sock);
            continue;
        }
        
        // 接收响应
        char buffer[4096];
        int n = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (n > 0) {
            buffer[n] = '\0';
            data->successful_requests++;
        } else {
            data->failed_requests++;
        }
        
        close(sock);
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        double elapsed = (end.tv_sec - start.tv_sec) + 
                        (end.tv_nsec - start.tv_nsec) / 1e9;
        data->total_time += elapsed;
        
        // 短暂延迟
        usleep(10000); // 10ms
    }
    
    return NULL;
}

int main() {
    printf("Starting performance test...\n");
    printf("Threads: %d, Requests per thread: %d\n", NUM_THREADS, REQUESTS_PER_THREAD);
    
    pthread_t threads[NUM_THREADS];
    thread_data_t thread_data[NUM_THREADS];
    
    // 初始化线程数据
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].successful_requests = 0;
        thread_data[i].failed_requests = 0;
        thread_data[i].total_time = 0;
    }
    
    // 记录开始时间
    struct timespec test_start, test_end;
    clock_gettime(CLOCK_MONOTONIC, &test_start);
    
    // 创建客户端线程
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, client_thread, &thread_data[i]);
    }
    
    // 等待所有线程完成
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // 记录结束时间
    clock_gettime(CLOCK_MONOTONIC, &test_end);
    double total_test_time = (test_end.tv_sec - test_start.tv_sec) + 
                            (test_end.tv_nsec - test_start.tv_nsec) / 1e9;
    
    // 统计结果
    int total_successful = 0;
    int total_failed = 0;
    double total_request_time = 0;
    
    for (int i = 0; i < NUM_THREADS; i++) {
        total_successful += thread_data[i].successful_requests;
        total_failed += thread_data[i].failed_requests;
        total_request_time += thread_data[i].total_time;
    }
    
    // 打印结果
    printf("\n========== Test Results ==========\n");
    printf("Total requests: %d\n", total_successful + total_failed);
    printf("Successful: %d\n", total_successful);
    printf("Failed: %d\n", total_failed);
    printf("Success rate: %.2f%%\n", 
           (total_successful * 100.0) / (total_successful + total_failed));
    printf("Total test time: %.2f seconds\n", total_test_time);
    printf("Requests per second: %.2f\n", total_successful / total_test_time);
    printf("Average request time: %.4f seconds\n", 
           total_request_time / total_successful);
    printf("==================================\n");
    
    return 0;
}