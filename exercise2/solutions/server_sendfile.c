#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <netinet/in.h>

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

    printf("Sendfile Server listening on port %d...\n", PORT);

    if ((client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
        perror("Accept failed");
        close(file_fd);
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Client connected! Starting file transfer...\n");

    // =========================================================================
    // THE SOLUTION: The Zero-Copy sendfile() Loop
    // Notice that there is NO user-space buffer allocated here!
    // =========================================================================
    off_t offset = 0;
    size_t bytes_to_send = 1073741824; // 1 GB in bytes
    ssize_t sent_bytes;

    // We loop because sendfile might not send the entire 1GB in one go.
    // The kernel updates the 'offset' pointer automatically on success.
    while (offset < (off_t)bytes_to_send) {
        sent_bytes = sendfile(client_fd, file_fd, &offset, bytes_to_send - offset);
        
        if (sent_bytes <= 0) {
            perror("sendfile failed or connection closed");
            break;
        }
    }

    printf("Transfer complete.\n");

    close(file_fd);
    close(client_fd);
    close(server_fd);

    return 0;
}