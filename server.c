#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>

/**
 * Project 1 starter code
 * All parts needed to be changed/added are marked with TODO
 */

#define BUFFER_SIZE 1024
#define DEFAULT_SERVER_PORT 8081
#define DEFAULT_REMOTE_HOST "131.179.176.34"
#define DEFAULT_REMOTE_PORT 5001

struct server_app {
    // Parameters of the server
    // Local port of HTTP server
    uint16_t server_port;

    // Remote host and port of remote proxy
    char *remote_host;
    uint16_t remote_port;
};

// The following function is implemented for you and doesn't need
// to be change
void parse_args(int argc, char *argv[], struct server_app *app);

// The following functions need to be updated
void handle_request(struct server_app *app, int client_socket);
void serve_local_file(int client_socket, const char *path);
void proxy_remote_file(struct server_app *app, int client_socket, const char *path);

// The main function is provided and no change is needed
int main(int argc, char *argv[])
{
    struct server_app app;
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    int ret;

    parse_args(argc, argv, &app);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(app.server_port);

    // The following allows the program to immediately bind to the port in case
    // previous run exits recently
    int optval = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) == -1) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", app.server_port);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == -1) {
            perror("accept failed");
            continue;
        }
        
        printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        handle_request(&app, client_socket);
        close(client_socket);
    }

    close(server_socket);
    return 0;
}

void parse_args(int argc, char *argv[], struct server_app *app)
{
    int opt;

    app->server_port = DEFAULT_SERVER_PORT;
    app->remote_host = NULL;
    app->remote_port = DEFAULT_REMOTE_PORT;

    while ((opt = getopt(argc, argv, "b:r:p:")) != -1) {
        switch (opt) {
        case 'b':
            app->server_port = atoi(optarg);
            break;
        case 'r':
            app->remote_host = strdup(optarg);
            break;
        case 'p':
            app->remote_port = atoi(optarg);
            break;
        default: /* Unrecognized parameter or "-?" */
            fprintf(stderr, "Usage: server [-b local_port] [-r remote_host] [-p remote_port]\n");
            exit(-1);
            break;
        }
    }

    if (app->remote_host == NULL) {
        app->remote_host = strdup(DEFAULT_REMOTE_HOST);
    }
}

void handle_request(struct server_app *app, int client_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Read the request from HTTP client
    // Note: This code is not ideal in the real world because it
    // assumes that the request header is small enough and can be read
    // once as a whole.
    // However, the current version suffices for our testing.
    bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        return;  // Connection closed or error
    }

    buffer[bytes_read] = '\0';
    // copy buffer to a new string
    char *request = malloc(strlen(buffer) + 1);
    strcpy(request, buffer);

    // Print the HTTP request header
    printf("HTTP request received:\n%s\n", request);

    // Parse the request to extract the requested file name or path
    char *file_name = strtok(request, " ");
    file_name = strtok(NULL, " ");
    // if the requested path is "/" (root), default to index.html
    if (strcmp(file_name, "/") == 0) {
        file_name = "index.html";
    }

    // Serve the requested file
    serve_local_file(client_socket, file_name);

    // Free allocated memory
    free(request);
}
void serve_local_file(int client_socket, const char *path) {
    FILE *file;
    char buffer[1024];
    size_t bytes_read;

    // Open the binary file in binary read mode
    file = fopen(path, "rb");
    if (file == NULL) {
        // If the file cannot be opened, send a 404 Not Found response
        const char *not_found_response = "HTTP/1.1 404 Not Found\r\n\r\n";
        send(client_socket, not_found_response, strlen(not_found_response), 0);
        return;
    }

    // Send HTTP response headers indicating success and binary content type
    const char *http_header = "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\n\r\n";
    send(client_socket, http_header, strlen(http_header), 0);

    // Read the file contents in chunks and send them to the client
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(client_socket, buffer, bytes_read, 0);
    }

    // Close the file
    fclose(file);
}


void proxy_remote_file(struct server_app *app, int client_socket, const char *path) {
    int remote_socket;
    struct sockaddr_in remote_addr;
    ssize_t bytes_sent, bytes_received;
    char buffer[1024];

    // Create a socket for the remote server
    remote_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (remote_socket == -1) {
        perror("socket creation failed");
        return;
    }

    // Set up the address structure for the remote server
    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(app->remote_port);
    if (inet_pton(AF_INET, app->remote_host, &remote_addr.sin_addr) <= 0) {
        perror("invalid address or address not supported");
        close(remote_socket);
        return;
    }

    // Connect to the remote server
    if (connect(remote_socket, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) < 0) {
        perror("connection to remote server failed");
        close(remote_socket);
        return;
    }

    // Forward the original request to the remote server
    bytes_sent = send(remote_socket, path, strlen(path), 0);
    if (bytes_sent <= 0) {
        perror("sending request to remote server failed");
        close(remote_socket);
        return;
    }

    // Receive the response from the remote server and pass it back to the client
    while ((bytes_received = recv(remote_socket, buffer, sizeof(buffer), 0)) > 0) {
        bytes_sent = send(client_socket, buffer, bytes_received, 0);
        if (bytes_sent <= 0) {
            perror("sending response to client failed");
            break;
        }
    }

    // Close the sockets
    close(remote_socket);
}