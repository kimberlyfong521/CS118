#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PORT 8081
#define BUF_SIZE 1024

void send_file(int client_socket, const char *filename) {
    char buffer[BUF_SIZE];
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        // File not found
        dprintf(client_socket, "HTTP/1.1 404 Not Found\r\n\r\n");
        return;
    }

    // Determine content type based on file extension
    const char *extension = strrchr(filename, '.');
    const char *content_type = "text/plain";
    if (extension != NULL) {
        if (strcmp(extension, ".html") == 0 || strcmp(extension, ".txt") == 0) {
            content_type = "text/html";
        } else if (strcmp(extension, ".jpg") == 0) {
            content_type = "image/jpeg";
        }
    }

    dprintf(client_socket, "HTTP/1.1 200 OK\r\n");
    dprintf(client_socket, "Content-Type: %s\r\n\r\n", content_type);

    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUF_SIZE, file)) > 0) {
        write(client_socket, buffer, bytes_read);
    }

    fclose(file);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char request[BUF_SIZE];

    // Create server socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Bind server socket to port
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Binding failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 10) == -1) {
        perror("Listening failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        // Accept incoming connection
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {
            perror("Accepting connection failed");
            close(server_socket);
            exit(EXIT_FAILURE);
        }

        // Receive HTTP request
        recv(client_socket, request, BUF_SIZE, 0);
        printf("Received request: %s\n", request);

        // Extract filename from request
        char filename[BUF_SIZE];
        sscanf(request, "GET /%s ", filename);

        // Send requested file
        send_file(client_socket, filename);

        // Close client socket
        close(client_socket);
    }

    // Close server socket
    close(server_socket);

    return 0;
}
