#ifndef ROUTER_H
#define ROUTER_H

typedef struct {
    const char* path;
    int (*handler)(int client_fd);
} route_t;

extern route_t routes[];
extern const int num_routes;

int handle_hello(int client_fd);

#endif