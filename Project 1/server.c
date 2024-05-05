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
    char *request = (char *)malloc(strlen(buffer) + 1);
    strcpy(request, buffer);
    const char *get_token = "GET /";
    char *file_name_start = strstr(request, get_token) + strlen(get_token);
    char *space_pos = strchr(file_name_start, ' ');
    size_t file_name_len;
    if(space_pos != NULL)
        file_name_len = space_pos - file_name_start;
    else
        file_name_len = strlen(file_name_start);

    char file_name[file_name_len + 1];
    strncpy(file_name, file_name_start, file_name_len);
    file_name[file_name_len] = '\0';

    // TODO: Parse the header and extract essential fields, e.g. file name
    // Hint: if the requested path is "/" (root), default to index.html
   // char file_name[] = "index.html";

    if (strcmp(file_name, "") == 0) {
    strcpy(file_name, "index.html");
    } 
    else {
        if ' ' in filename or '%' in filename:
            new_filename = filename.replace(' ', '').replace('%', '')
            os.rename(os.path.join(directory, filename), os.path.join(directory, new_filename))
    }

    // TODO: Implement proxy and call the function under condition
    // specified in the spec
    // if (need_proxy(...)) {
    //    proxy_remote_file(app, client_socket, file_name);
    // } else {
    
    char remoteEnd[] = ".ts";
    size_t remoteEndLen = strlen(remoteEnd);

    if (!(strlen(file_name) >= 3 &&
        strncmp(remoteEnd, file_name + strlen(file_name) - remoteEndLen, remoteEndLen) == 0)) {
        serve_local_file(client_socket, file_name);
    } else {
        proxy_remote_file(app, client_socket, request);
    }
    
    }

void serve_local_file(int client_socket, const char *path) {
    // TODO: Properly implement serving of local files
    // The following code returns a dummy response for all requests
    // but it should give you a rough idea about what a proper response looks like
    // What you need to do 
    // (when the requested file exists):
    // * Open the requested file
    // * Build proper response headers (see details in the spec), and send them
    // * Also send file content
    // (When the requested file does not exist):
    // * Generate a correct response

    FILE *file = fopen(path, "rb");
    if (!file) {
        char response[] =
            "HTTP/1.0 404 Not Found\r\n"
            "Content-Type: text/plain; charset=UTF-8\r\n"
            "Content-Length: 13\r\n"
            "\r\n"
            "404 Not Found";
        send(client_socket, response, strlen(response), 0);
        return;
    }

    const char *mime_type_start = NULL;
    for (int i = 0; path[i] != '\0'; ++i) {
        if (path[i] == '.') {
            mime_type_start = &path[i];
        }
    }

    char content_type[50] = "application/octet-stream";
    if (mime_type_start != NULL) {
        char mime_type[10];
        strcpy(mime_type, mime_type_start + 1);
        if (strcmp(mime_type, "txt") == 0) {
            strcpy(content_type, "text/plain; charset=UTF-8");
        } else if (strcmp(mime_type, "html") == 0) {
            strcpy(content_type, "text/html");
        } else if (strcmp(mime_type, "jpg") == 0) {
            strcpy(content_type, "image/jpeg");
        }
    }

    char response[1024];
    sprintf(response, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\n", content_type);

    fseek(file, 0, SEEK_END);
    long file_length = ftell(file);
    fseek(file, 0, SEEK_SET);
    sprintf(response + strlen(response), "Content-Length: %ld\r\n\r\n", file_length);

    send(client_socket, response, strlen(response), 0);

    char buffer[1024];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(client_socket, buffer, bytes_read, 0);
    }

    fclose(file);
}
void proxy_remote_file(struct server_app *app, int client_socket, const char *request) {
    // TODO: Implement proxy request and replace the following code
    // What's needed:
    // * Connect to remote server (app->remote_server/app->remote_port)
    // * Forward the original request to the remote server
    // * Pass the response from remote server back
    // Bonus:
    // * When connection to the remote server fail, properly generate
    // HTTP 502 "Bad Gateway" response

    int remote_socket;
    struct sockaddr_in remote_addr;

    remote_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (remote_socket == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(app->remote_port);
    remote_addr.sin_addr.s_addr = inet_addr(app->remote_host);

    int status = connect(remote_socket, (struct sockaddr *)&remote_addr, sizeof(remote_addr));
    if (status == -1) {
        char response[] = "HTTP/1.0 502 BAD GATEWAY\r\n\r\n";
        send(client_socket, response, strlen(response), 0);
        return;
    }

    send(remote_socket, request, strlen(request), 0);
    printf("Forwarding Request To Remote Server\n");

    char buff[BUFFER_SIZE];
    int bytesRead, bytesSent;
    while ((bytesRead = read(remote_socket, buff, BUFFER_SIZE)) > 0) {
        bytesSent = send(client_socket, buff, bytesRead, 0);
        if (bytesSent < 0) {
            perror("Failed to Send");
            break;
        }
    }

    if (bytesRead < 0) {
        perror("Failed to Read");
    }

    close(remote_socket);
}