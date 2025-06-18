#include "router.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>

#define BUFFER_SIZE 4096

route_t get_routes[] = {
    {"/api/hello", handle_hello},
};

int get_routes_num = 1;

int handle_hello(int client_fd) {
    const char *body = "{\"message\": \"Hello from router!\"}";
    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %lu\r\n"
        "\r\n"
        "%s", strlen(body), body);

    send(client_fd, response, strlen(response), 0);
    return 200;
}
