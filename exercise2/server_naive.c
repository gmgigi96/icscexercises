#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define BUFFER_SIZE 65536 // 64KB buffer to ensure many system calls

int main() {
    int server_fd, client_fd, file_fd;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read, bytes_written;

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

    printf("Naive Server listening on port %d...\n", PORT);

    if ((client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
        perror("Accept failed");
        close(file_fd);
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Client connected! Starting file transfer...\n");

    // =========================================================================
    // The Naive Read/Write Loop
    // This loop causes the "Double Copy" and pays the "Syscall Tax"
    // =========================================================================
    while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
        // We read from the disk to user space, now we write from user space to the socket
        bytes_written = write(client_fd, buffer, bytes_read);
        
        if (bytes_written < 0) {
            perror("Error writing to socket");
            break;
        }
    }

    if (bytes_read < 0) {
        perror("Error reading from file");
    }

    printf("Transfer complete.\n");

    close(file_fd);
    close(client_fd);
    close(server_fd);

    return 0;
}