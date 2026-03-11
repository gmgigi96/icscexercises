#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <filename> <buffer_size_in_bytes>\n", argv[0]);
        return 1;
    }

    char *filename = argv[1];
    size_t buffer_size = strtoull(argv[2], NULL, 10);

    if (buffer_size == 0) {
        fprintf(stderr, "Buffer size must be greater than 0.\n");
        return 1;
    }

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        return 1;
    }

    char *buffer = malloc(buffer_size);
    if (!buffer) {
        perror("Error allocating memory");
        close(fd);
        return 1;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Core I/O Loop
    ssize_t bytes_read;
    size_t total_bytes = 0;
    
    while ((bytes_read = read(fd, buffer, buffer_size)) > 0) {
        total_bytes += bytes_read;
    }

    if (bytes_read < 0) {
        perror("Error reading file");
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Read %zu bytes using a %zu-byte buffer.\n", total_bytes, buffer_size);
    printf("Time elapsed: %.4f seconds\n", elapsed_time);
    printf("Throughput:   %.2f MB/s\n", (total_bytes / (1024.0 * 1024.0)) / elapsed_time);

    free(buffer);
    close(fd);
    return 0;
}