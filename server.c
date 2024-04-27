#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8081
#define BUFFER_SIZE 1024

void handle_request(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    int valread = read(client_socket, buffer, BUFFER_SIZE);
    if (valread > 0) {
        printf("Request received:\n%s\n", buffer);

        // Extract the requested file path from the HTTP request
        char *token = strtok(buffer, " ");
        if (token != NULL) {
            token = strtok(NULL, " "); // Second token is the path
            if (token != NULL) {
                // Remove leading '/' from the file path
                char *file_path = token + 1;

                // Open the requested file
                FILE *file = fopen(file_path, "rb");
                if (file != NULL) {
                    // Determine content type based on file extension
                    const char *content_type;
                    if (strstr(file_path, ".html") || strstr(file_path, ".txt")) {
                        content_type = "text/html";
                    } else if (strstr(file_path, ".jpg")) {
                        content_type = "image/jpeg";
                    } else {
                        content_type = "application/octet-stream";
                    }

                    // Send HTTP response header
                    fprintf(stdout, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", content_type);
                    fflush(stdout);

                    // Send file contents
                    char response_buffer[BUFFER_SIZE];
                    size_t bytes_read;
                    while ((bytes_read = fread(response_buffer, 1, sizeof(response_buffer), file)) > 0) {
                        send(client_socket, response_buffer, bytes_read, 0);
                    }

                    fclose(file);
                } else {
                    // File not found
                    char not_found_response[] = "HTTP/1.1 404 Not Found\r\n\r\nFile Not Found";
                    send(client_socket, not_found_response, strlen(not_found_response), 0);
                }
            }
        }
    }
    close(client_socket);
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket to address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("Server listening on port %d\n", PORT);

    while (1) {
        // Accept incoming connection
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Handle client request
        handle_request(client_socket);
    }

    return 0;
}
