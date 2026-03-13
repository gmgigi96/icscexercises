#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
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

    printf("Splice Server listening on port %d...\n", PORT);

    if ((client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
        perror("Accept failed");
        close(file_fd);
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Client connected! Starting file transfer...\n");

    int pipefds[2];
    if (pipe(pipefds) < 0) {
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }

    fcntl(pipefds[0], F_SETPIPE_SZ, 1024 * 1024);
    size_t chunk_size = 1024 * 1024; // 1MB chunks

    size_t bytes_sent = 0;
    size_t file_size = 1073741824; // 1 GB

    while (bytes_sent < file_size) {
        // Pull data from disk into the kernel pipe
        ssize_t spliced_in = splice(file_fd, NULL, pipefds[1], NULL, chunk_size, SPLICE_F_MORE);
        
        if (spliced_in <= 0) break; // Error or EOF

        // Push data from the kernel pipe into the network socket
        ssize_t spliced_out = splice(pipefds[0], NULL, client_fd, NULL, spliced_in, SPLICE_F_MORE);
        
        if (spliced_out <= 0) break;

        bytes_sent += spliced_out;
    }

    printf("Transfer complete.\n");

    close(pipefds[0]);
    close(pipefds[1]);
    close(file_fd);
    close(client_fd);
    close(server_fd);

    return 0;
}