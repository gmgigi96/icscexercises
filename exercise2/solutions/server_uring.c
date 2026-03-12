#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <liburing.h> // REQUIRED FOR IO_URING

#define PORT 8080
#define CHUNK_SIZE (256 * 1024 * 1024) // 256 MB chunks
#define TOTAL_SIZE (1024 * 1024 * 1024) // 1 GB total

#define PORT 8080

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
    // THE SOLUTION: io_uring with IOSQE_IO_LINK (I/O Chaining)
    // =========================================================================
    
    // Allocate the large user-space buffer (Note: This guarantees a Double Copy!)
    char *buffer = malloc(CHUNK_SIZE);
    if (!buffer) {
        perror("Failed to allocate 256MB buffer");
        exit(EXIT_FAILURE);
    }

    // Initialize the ring with an 8-entry queue
    struct io_uring ring;
    if (io_uring_queue_init(8, &ring, 0) < 0) {
        perror("io_uring_queue_init failed");
        exit(EXIT_FAILURE);
    }

    off_t offset = 0;

    // We process the 1GB file in four 256MB chunks
    while (offset < TOTAL_SIZE) {
        
        // --- 1. PREPARE THE READ ---
        struct io_uring_sqe *sqe1 = io_uring_get_sqe(&ring);
        io_uring_prep_read(sqe1, file_fd, buffer, CHUNK_SIZE, offset);
        
        // This flag is the magic: "Don't stop, do the next SQE immediately!"
        io_uring_sqe_set_flags(sqe1, IOSQE_IO_LINK);
        sqe1->user_data = 1; // Tag it so we know which CQE belongs to the read

        // --- 2. PREPARE THE WRITE ---
        struct io_uring_sqe *sqe2 = io_uring_get_sqe(&ring);
        // Offset is 0 for network sockets, as they are non-seekable streams
        io_uring_prep_write(sqe2, client_fd, buffer, CHUNK_SIZE, 0); 
        sqe2->user_data = 2; // Tag it so we know which CQE belongs to the write

        // --- 3. RING THE DOORBELL ---
        // Submit both the read and the write with a SINGLE system call
        io_uring_submit(&ring);

        // --- 4. WAIT FOR THE RECEIPTS ---
        // We expect exactly 2 completions (one for read, one for write)
        for (int i = 0; i < 2; i++) {
            struct io_uring_cqe *cqe;
            io_uring_wait_cqe(&ring, &cqe);
            
            if (cqe->res < 0) {
                fprintf(stderr, "Async I/O failed! Error code: %d\n", cqe->res);
            }
            
            // Mark this CQE as seen so the kernel can recycle the slot
            io_uring_cqe_seen(&ring, cqe);
        }

        offset += CHUNK_SIZE;
    }

    printf("Transfer complete.\n");
    
    io_uring_queue_exit(&ring);
    free(buffer);
    close(file_fd);
    close(client_fd);
    close(server_fd);

    return 0;
}