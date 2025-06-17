#ifndef REQUEST_H
#define REQUEST_H

#include <netinet/in.h>
#include <arpa/inet.h>

void *handle_client_req(void *arg);

typedef struct {
    int client_fd;
    struct sockaddr_in client_addr;
} client_info_t;

#endif