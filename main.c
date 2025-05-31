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

int main()
{

    // File descriptors for server and client addreses
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    


    // Creating TCP Socket: AF_INET - IPv4 address (alt. AF_INET6), SOCK_STREAM - TCP stream (alt. SOCK_DGRAM)
    // 0 - default protocol 0 = IPPROTO_TCP
    // On success returns socket file descriptor, on failure -1 and error
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

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


    printf("Server is listening on port %d...\n", PORT);

    char buffer[BUFFER_SIZE];
    char http_response[BUFFER_SIZE];
    while (1)
    {
        printf("Waiting for a new connection...\n");

        // Accepting a client. Creating socket for the client (client_fd).
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0)
        {
            perror("accept failed");
            continue;
        }

        printf("Connection accepted from %s:%d\n",
            inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
       
        // Handling Request
        int bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0); 
        if(bytes_received < 0) {
            perror("recv failed");
            close(client_fd);
            continue;
        }
        buffer[bytes_received] = '\0';
        
        // Parsing request
        char method[8], path[1024], version[16];
        sscanf(buffer, "%7s %1023s %15s", method, path, version);
        printf("--- Parsed Request ---\n");
        printf("Method: %s\n", method);
        printf("Path: %s\n", path);
        printf("Version: %s\n", version);

        // Sending response
        const char *body;
        if(strcmp(path, "/") == 0) {
            body = "This is homepage";
        } else if (strcmp(path, "/hello") == 0) {
            body = "Hello friend";
        } else {
            body = "404 Not Found";
        }
    
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

    return 0;
}