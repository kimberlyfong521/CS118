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
    // Open the requested file
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        // If the file does not exist, send HTTP 404 response
        char response[] = "HTTP/1.0 404 Not Found\r\n\r\n";
        send(client_socket, response, strlen(response), 0);
        return;
    }

    // Get file size
    fseek(file, 0L, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    // Determine Content-Type based on file extension
    const char *content_type;
    if (strstr(path, ".html") || strstr(path, ".txt")) {
        content_type = "text/html; charset=UTF-8";
    } else if (strstr(path, ".jpg")) {
        content_type = "image/jpeg";
    } else {
        content_type = "application/octet-stream";
    }

    // Prepare HTTP response headers
    char headers[BUFFER_SIZE];
    snprintf(headers, sizeof(headers), "HTTP/1.0 200 OK\r\n"
                                       "Content-Type: %s\r\n"
                                       "Content-Length: %ld\r\n"
                                       "\r\n",
                                       content_type, file_size);

    // Send HTTP response headers
    send(client_socket, headers, strlen(headers), 0);

    // Send file content
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(client_socket, buffer, bytes_read, 0);
    }

    // Close the file
    fclose(file);
}

void proxy_remote_file(struct server_app *app, int client_socket, const char *path) {
    // This function should be implemented for reverse proxy functionality
    // It is not needed for serving local files in a simple web server
    // You can leave it empty or implement it according to your project requirements
    // or specifications.
    // If implementing reverse proxy, you would connect to the remote server,
    // forward the original request, and pass the response back to the client.
}

