#include "server.h"
#include "request.h"

#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

static int server_fd;

void setup_server_socket(int PORT, int BACKLOG);

void server_start(int PORT, int BACKLOG) {
    setup_server_socket(PORT, BACKLOG);

    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    printf("Server is listening on port %d...\n", PORT);

    while (1)
    {
        printf("Waiting for a new connection...\n");

        // Accepting a client. Creating socket for the client (client_fd)
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
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
}

void setup_server_socket(int PORT, int BACKLOG) {
    // Creating TCP Socket: AF_INET - IPv4 address (alt. AF_INET6), SOCK_STREAM - TCP stream (alt. SOCK_DGRAM)
    // 0 - default protocol 0 = IPPROTO_TCP
    // On success returns socket file descriptor, on failure -1 and error
    int server_fd_tmp = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd_tmp == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;

    // Binding socket to port
    memset(&server_addr, 0, sizeof(server_addr)); // clear server_addr structure
    server_addr.sin_family = AF_INET;             // IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY;     // INADDR_ANY=0.0.0.0, listen on all interfaces (Ethernet, WiFi, localhost etc.)
    server_addr.sin_port = htons(PORT);           // converting to network byte order (Big Endian)

    if (bind(server_fd_tmp, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        close(server_fd_tmp);
        exit(EXIT_FAILURE);
    }

    // Listening for connections. Backlog - number/queue of connections that can wait for calling accept()
    // listen() mark our socket as a server and set queue size. As before: ret < 0 = error
    if (listen(server_fd_tmp, BACKLOG) < 0)
    {
        perror("listen failed");
        close(server_fd_tmp);
        exit(EXIT_FAILURE);
    }
    server_fd = server_fd_tmp;
}

void server_close() {
    close(server_fd);
}