#ifndef ROUTER_H
#define ROUTER_H

typedef struct {
    const char* path;
    int (*handler)(int client_fd);
} route_t;

typedef struct {
    const char* path;
    const char* filename;
} static_route_t;

struct request_t {
    int client_fd;
    char* method;
    char* path;
    char* buffer;
    int bytes_received;
};

extern route_t get_routes[];
extern static_route_t static_routes[];
extern int get_routes_num;
extern int static_routes_num;

int handle_hello(int client_fd);
char *create_response(char *body);
void route(struct request_t *req, int *response_code);

#endif