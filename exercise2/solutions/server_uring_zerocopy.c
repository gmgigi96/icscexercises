#define _GNU_SOURCE // for F_SETPIPE_SZ

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <liburing.h> // REQUIRED FOR IO_URING

#define PORT 8080
#define CHUNK_SIZE (1024 * 1024)        // 1 MB chunks
#define TOTAL_SIZE (1024 * 1024 * 1024) // 1 GB total

int main() {
    int server_fd, client_fd, file_fd;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);

    file_fd = open("1GB_dataset.bin", O_RDONLY);
    if (file_fd < 0) {
        perror("Failed to open 1GB_dataset.bin (Did you run the dd command?)");
        exit(EXIT_FAILURE);
    }

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        close(file_fd);
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        close(file_fd);
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(file_fd);
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(file_fd);
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Uring Server listening on port %d...\n", PORT);

    if ((client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
        perror("Accept failed");
        close(file_fd);
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Client connected! Starting file transfer...\n");

    // =========================================================================
    // THE SOLUTION: Zero copy io_uring
    // =========================================================================

    int pipefds[2];
    if (pipe(pipefds) < 0) {
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }

    // Increase pipe size to 1MB
    fcntl(pipefds[0], F_SETPIPE_SZ, CHUNK_SIZE);

    struct io_uring ring;
    if (io_uring_queue_init(8, &ring, 0) < 0) {
        perror("io_uring_queue_init failed");
        exit(EXIT_FAILURE);
    }

    off_t offset = 0;

    // We process the 1GB file in 64KB chunks
    while (offset < TOTAL_SIZE) {
        
        // --- SQE 1: Splice from file into the kernel pipe ---
        struct io_uring_sqe *sqe1 = io_uring_get_sqe(&ring);
        // fd_in, off_in, fd_out, off_out, nbytes, flags
        io_uring_prep_splice(sqe1, file_fd, offset, pipefds[1], -1, CHUNK_SIZE, 0);
        
        // CHAIN IT!
        io_uring_sqe_set_flags(sqe1, IOSQE_IO_LINK);

        // --- SQE 2: Splice from the kernel pipe into the network socket ---
        struct io_uring_sqe *sqe2 = io_uring_get_sqe(&ring);
        io_uring_prep_splice(sqe2, pipefds[0], -1, client_fd, -1, CHUNK_SIZE, 0);

        // Submit both linked operations
        io_uring_submit(&ring);

        // Wait for the 2 CQEs
        for (int i = 0; i < 2; i++) {
            struct io_uring_cqe *cqe;
            io_uring_wait_cqe(&ring, &cqe);
            
            if (cqe->res < 0) {
                fprintf(stderr, "Async Splice failed! Error code: %d\n", cqe->res);
            }

            io_uring_cqe_seen(&ring, cqe);
        }

        offset += CHUNK_SIZE;
    }

    printf("Zero-copy transfer complete.\n");
    
    io_uring_queue_exit(&ring);
    close(file_fd);
    close(client_fd);
    close(server_fd);
    // Don't forget to close the pipes!
    close(pipefds[0]);
    close(pipefds[1]);

    return 0;
}