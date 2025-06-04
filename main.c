#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h> // files size
#include <ctype.h> // is digit function

#include "http.h"
#include "request.h"
#include "utils.h"

#define PORT 8080
#define BACKLOG 10
#define BUFFER_SIZE 4096

int setup_server_socket();
void* handle_client_req(void* args);

typedef struct {
    int client_fd;
    struct sockaddr_in client_addr;
} client_info_t;

int main() {
    int server_fd = setup_server_socket(); 

    // File descriptors for server and client addreses
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd;

    printf("Server is listening on port %d...\n", PORT);

    while (1)
    {
        printf("Waiting for a new connection...\n");

        // Accepting a client. Creating socket for the client (client_fd)
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0)
        {
            perror("accept failed");
            continue;
        }

        client_info_t *client_info = malloc(sizeof(client_info_t));
        if(!client_info) {
            perror("malloc failed");
            close(client_fd);
            continue;
        }

        client_info->client_fd = client_fd;
        client_info->client_addr = client_addr;

        //handling request asynchronously
        pthread_t thread_id;
        if(pthread_create(&thread_id, NULL, handle_client_req, client_info) != 0) {
            perror("pthread_create failed");
            close(client_fd);
            free(client_info);
        } else {
            pthread_detach(thread_id); // cleanup thread after it ends
        }
    }

    close(server_fd);
    return 0;
}

int setup_server_socket() {
    // Creating TCP Socket: AF_INET - IPv4 address (alt. AF_INET6), SOCK_STREAM - TCP stream (alt. SOCK_DGRAM)
    // 0 - default protocol 0 = IPPROTO_TCP
    // On success returns socket file descriptor, on failure -1 and error
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;

    // Binding socket to port
    memset(&server_addr, 0, sizeof(server_addr)); // clear server_addr structure
    server_addr.sin_family = AF_INET;             // IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY;     // INADDR_ANY=0.0.0.0, listen on all interfaces (Ethernet, WiFi, localhost etc.)
    server_addr.sin_port = htons(PORT);           // converting to network byte order (Big Endian)
    
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Listening for connections. Backlog - number/queue of connections that can wait for calling accept()
    // listen() mark our socket as a server and set queue size. As before: ret < 0 = error
    if (listen(server_fd, BACKLOG) < 0)
    {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    return server_fd;
}

void* handle_client_req(void* arg) {
    // parse client_fd and client_addr
    client_info_t *client_info = (client_info_t*)arg;
    int client_fd = client_info->client_fd;
    struct sockaddr_in client_addr = client_info->client_addr;
    free(client_info);

    printf("Connection accepted from %s:%d\n",
            inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // copy received data into buffer
    char buffer[BUFFER_SIZE];
    char http_response[BUFFER_SIZE];
    int bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0); 
    if(bytes_received < 0) {
        perror("recv failed");
        close(client_fd);
        return NULL;
    }
    buffer[bytes_received] = '\0';
    
    // parse data from buffer
    char method[8], path[1024], version[16];
    parse_request(buffer, method, path, version);
    int response_code;
    const char* user_agent = get_header_value(buffer, "User-Agent");

    if(strcmp(method, "GET") == 0) {
        response_code = handle_get(client_fd, path, 1);
    } else if (strcmp(method, "HEAD") == 0){
        response_code = handle_get(client_fd, path, 0);
    } else if(strcmp(method, "POST") == 0) {
        response_code = handle_post(client_fd, buffer, bytes_received);
    } else if(strcmp(method, "OPTIONS") == 0) {
        response_code = handle_options(client_fd);
    } else {
        const char *body = "405 Method Not Allowed";
        response_code = 405;
        snprintf(http_response, sizeof(http_response),
            "HTTP/1.1 405 Method Not Allowed\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %lu\r\n"
            "Allow: GET, POST\r\n"
            "\r\n"
            "%s",
            strlen(body),
            body
        );
        send(client_fd, http_response, strlen(http_response), 0);
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    
    log_request(client_ip, method, path, version, response_code, user_agent);

    close(client_fd);
    return NULL;
}