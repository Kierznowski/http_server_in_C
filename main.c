#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 8080
#define BACKLOG 10
#define BUFFER_SIZE 4096

int setup_server_socket();
void handle_client_request(int client_fd, struct sockaddr_in client_addr);
void parse_request(const char *buffer, char *method, char *path, char *version);
const char* route_path(const char* path);

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

        handle_client_request(client_fd, client_addr);
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

void handle_client_request(int client_fd, struct sockaddr_in client_addr) {
    char buffer[BUFFER_SIZE];
    char http_response[BUFFER_SIZE];

    printf("Connection accepted from %s:%d\n",
            inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    int bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0); 
    if(bytes_received < 0) {
        perror("recv failed");
        close(client_fd);
        return;
    }
    buffer[bytes_received] = '\0';
        
    char method[8], path[1024], version[16];
    parse_request(buffer, method, path, version);

    log_request(method, path, version);

    const char *body = route_path(path);
    
    snprintf(http_response, sizeof(http_response),
        "HTTP/1.1 %s\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %lu\r\n"
        "\r\n"
        "%s",
        (strcmp(body, "404 Not Found") == 0) ? "404 Not Found" : "200 OK",
        strlen(body),
        body
    );

    send(client_fd, http_response, strlen(http_response), 0);
    close(client_fd);
}

void parse_request(const char *buffer, char *method, char *path, char *version) {
    sscanf(buffer, "%7s %1023s %15s", method, path, version);
}

const char* route_path(const char* path) {
    if(strcmp(path, "/") == 0) {
        return "This is homepage";
    } else if (strcmp(path, "/hello") == 0) {
        return "Hello friend";
    } else {
        return "404 Not Found";
    }
}

void log_request(const char* method, const char* path, const char* version) {
    printf("--- Parsed Request ---\n");
    printf("Method: %s\n", method);
    printf("Path: %s\n", path);
    printf("Version: %s\n", version);
}