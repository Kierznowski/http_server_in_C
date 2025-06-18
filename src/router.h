#ifndef ROUTER_H
#define ROUTER_H

typedef struct {
    const char* path;
    int (*handler)(int client_fd);
} route_t;

extern route_t get_routes[];
extern int get_routes_num;

int handle_hello(int client_fd);

#endif