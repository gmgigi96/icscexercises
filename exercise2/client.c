#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 65536 // 64KB buffer to drain the socket quickly

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    size_t total_bytes = 0;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address / Address not supported");
        exit(EXIT_FAILURE);
    }

    printf("Connecting to server at 127.0.0.1:%d...\n", PORT);
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected! Receiving data...\n");

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    while ((bytes_received = read(sock, buffer, sizeof(buffer))) > 0) {
        total_bytes += bytes_received;
    }

    if (bytes_received < 0) {
        perror("Read error");
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    double mb_received = (double)total_bytes / (1024 * 1024);
    printf("Transfer complete. Received %.2f MB.\n", mb_received);
    printf("Throughput:   %.2f MB/s\n", mb_received / elapsed_time);

    close(sock);

    return 0;
}