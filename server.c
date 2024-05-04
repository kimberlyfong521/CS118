#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h> 

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
   // Parse the HTTP request header to extract the requested file name
    char *file_name = NULL;
    char *request_line = strtok(buffer, "\r\n"); // Extract the first line of the request
    if (request_line != NULL) {
        // Find start of file path after get
        char *path_start = strstr(request_line, "GET ");
        if (path_start != NULL) {
            path_start += strlen("GET "); // move pointer to start of path
            // Find end of path
            char *path_end = strchr(path_start, ' ');
            if (path_end != NULL) {
                // Allocate memory for the file name
                file_name = malloc(path_end - path_start + 1);
                if (file_name != NULL) {
                    // Copy the file path to the file_name variable
                    strncpy(file_name, path_start, path_end - path_start);
                    file_name[path_end - path_start] = '\0';
                }
            }
        }
    }

    // default index.html
    if (file_name == NULL || strcmp(file_name, "/") == 0) {
        file_name = strdup("index.html");
    }

    // Decide whether to serve the file locally or proxy the request
    if (strcasecmp(file_name + strlen(file_name) - 4, ".txt") == 0 ||
        strcasecmp(file_name + strlen(file_name) - 5, ".html") == 0 ||
        strcasecmp(file_name + strlen(file_name) - 4, ".jpg") == 0) {
        proxy_remote_file(app, client_socket, file_name);
    } else {
        serve_local_file(client_socket, file_name);
    }

    // Free allocated memory for file_name
    free(file_name);
}

void serve_local_file(int client_socket, const char *path) {
    // Open the requested file
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        // If the file does not exist, generate a correct response
        char response[] = "HTTP/1.0 404 Not Found\r\n"
                          "Content-Type: text/html\r\n"
                          "\r\n"
                          "<html><body><h1>404 Not Found</h1></body></html>";
        send(client_socket, response, strlen(response), 0);
        return;
    }

    // Get the file size
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Build proper response headers
    char headers[BUFFER_SIZE];
    snprintf(headers, BUFFER_SIZE, "HTTP/1.0 200 OK\r\n"
                                    "Content-Type: text/plain; charset=UTF-8\r\n"
                                    "Content-Length: %zu\r\n"
                                    "\r\n", file_size);
    send(client_socket, headers, strlen(headers), 0);

    // Send file content
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        send(client_socket, buffer, bytes_read, 0);
    }

    fclose(file);
}


void proxy_remote_file(struct server_app *app, int client_socket, const char *request) {
    // Connect to the remote server
    int backend_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (backend_socket == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in backend_addr;
    memset(&backend_addr, 0, sizeof(backend_addr));
    backend_addr.sin_family = AF_INET;
    backend_addr.sin_port = htons(app->remote_port);

    struct hostent *backend_host = gethostbyname(app->remote_host);
    if (backend_host == NULL) {
        perror("gethostbyname failed");
        exit(EXIT_FAILURE);
    }
    memcpy(&backend_addr.sin_addr.s_addr, backend_host->h_addr, backend_host->h_length);

    if (connect(backend_socket, (struct sockaddr *)&backend_addr, sizeof(backend_addr)) == -1) {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }

    // Forward the original request to the remote server
    if (send(backend_socket, request, strlen(request), 0) == -1) {
        perror("send failed");
        exit(EXIT_FAILURE);
    }

    // Pass the response from the remote server back to the client
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    while ((bytes_received = recv(backend_socket, buffer, sizeof(buffer), 0)) > 0) {
        if (send(client_socket, buffer, bytes_received, 0) == -1) {
            perror("send failed");
            exit(EXIT_FAILURE);
        }
    }
    //failll
    if (bytes_received == -1) {
        perror("recv failed");
        exit(EXIT_FAILURE);
    }

    close(backend_socket);
}
