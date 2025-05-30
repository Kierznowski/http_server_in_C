#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 8080
#define BACKLOG 10

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

    // Accepting a client. Creating socket for the client (client_fd).
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
    if (client_fd < 0)
    {
        perror("accept failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Connection accepted from %s:%d\n",
        inet_ntoa(client_addr.sin_addr), // converting binary IP address to "xxx.xxx.xxx.xxx" form
        ntohs(client_addr.sin_port));

    // Close sockets
    close(client_fd);
    close(server_fd);

    return 0;
}