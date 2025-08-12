#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define NUM_THREADS 10
#define REQUESTS_PER_THREAD 100

typedef struct {
    int thread_id;
    int successful_requests;
    int failed_requests;
    int connect_failures;
    int send_failures;
    int recv_failures;
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
            data->connect_failures++;
            close(sock);
            usleep(1000); // 1ms延迟后重试避免连接风暴
            continue;
        }
        
        // 发送 HTTP 请求
        const char* request = "GET / HTTP/1.1\r\n"
                             "Host: localhost\r\n"
                             "Connection: keep-alive\r\n"
                             "\r\n";
        
        if (send(sock, request, strlen(request), 0) < 0) {
            data->failed_requests++;
            data->send_failures++;
            close(sock);
            usleep(500); // 0.5ms延迟
            continue;
        }
        
        // 设置接收超时
        struct timeval timeout;
        timeout.tv_sec = 1;  // 1秒超时
        timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        
        // 设置TCP_NODELAY以减少延迟
        int flag = 1;
        setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
        
        // 设置发送超时
        struct timeval send_timeout;
        send_timeout.tv_sec = 1;
        send_timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, sizeof(send_timeout));
        
        // 接收响应（带快速重试）
        char buffer[4096];
        int n = recv(sock, buffer, sizeof(buffer) - 1, 0);
        
        // 如果首次接收失败，快速重试一次（不额外等待）
        if (n <= 0) {
            n = recv(sock, buffer, sizeof(buffer) - 1, 0);
        }
        
        if (n > 0) {
            buffer[n] = '\0';
            data->successful_requests++;
        } else {
            data->failed_requests++;
            data->recv_failures++;
        }
        
        close(sock);
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        double elapsed = (end.tv_sec - start.tv_sec) + 
                        (end.tv_nsec - start.tv_nsec) / 1e9;
        data->total_time += elapsed;
        
        // 短暂延迟以避免过载服务器，基于线程ID错开
        if (i % 10 == 0) { // 每10个请求稍作休息
            usleep(1000); // 1ms
        } else {
            usleep(50); // 0.05ms
        }
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
        thread_data[i].connect_failures = 0;
        thread_data[i].send_failures = 0;
        thread_data[i].recv_failures = 0;
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
    int total_connect_failures = 0;
    int total_send_failures = 0;
    int total_recv_failures = 0;
    double total_request_time = 0;
    
    for (int i = 0; i < NUM_THREADS; i++) {
        total_successful += thread_data[i].successful_requests;
        total_failed += thread_data[i].failed_requests;
        total_connect_failures += thread_data[i].connect_failures;
        total_send_failures += thread_data[i].send_failures;
        total_recv_failures += thread_data[i].recv_failures;
        total_request_time += thread_data[i].total_time;
    }
    
    // 打印结果
    printf("\n========== Test Results ==========\n");
    printf("Total requests: %d\n", total_successful + total_failed);
    printf("Successful: %d\n", total_successful);
    printf("Failed: %d\n", total_failed);
    if (total_failed > 0) {
        printf("  - Connect failures: %d\n", total_connect_failures);
        printf("  - Send failures: %d\n", total_send_failures);
        printf("  - Recv failures: %d\n", total_recv_failures);
    }
    printf("Success rate: %.2f%%\n", 
           (total_successful * 100.0) / (total_successful + total_failed));
    printf("Total test time: %.2f seconds\n", total_test_time);
    printf("Requests per second: %.2f\n", total_successful / total_test_time);
    printf("Average request time: %.4f seconds\n", 
           total_request_time / total_successful);
    printf("==================================\n");
    
    return 0;
}