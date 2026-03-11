#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    // Use fopen instead of open
    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    size_t total_bytes = 0;
    char buffer[1]; // Reading 1 byte at a time!

    // Use fread instead of read
    while (fread(buffer, 1, 1, file) == 1) {
        total_bytes++;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Read %zu bytes using fgetc (1-byte application reads).\n", total_bytes);
    printf("Time elapsed: %.4f seconds\n", elapsed_time);
    printf("Throughput:   %.2f MB/s\n", (total_bytes / (1024.0 * 1024.0)) / elapsed_time);

    fclose(file);
    return 0;
}