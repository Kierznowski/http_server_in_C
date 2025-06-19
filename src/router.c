#include "router.h"
#include "http.h"
#include "http_errors.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>

#define BUFFER_SIZE 4096

route_t get_routes[] = {
    {"/api/hello", handle_hello},
};

static_route_t static_routes[] = {
    {"/", "index.html"},
};

int get_routes_num = 1;
int static_routes_num = 1;

int route_get(int client_fd, const char* path, int send_body);
int serve_static_file(int client_fd, const char* path, int send_body);

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

void route(struct request_t *req, int *response_code) {
    if(strcmp(req->method, "GET") == 0) {
        *response_code = route_get(req->client_fd, req->path, 1);
    } else if (strcmp(req->method, "HEAD") == 0){
        *response_code = route_get(req->client_fd, req->path, 0);
    } else if(strcmp(req->method, "POST") == 0) {
        *response_code = handle_post(req->client_fd, req->buffer, req->bytes_received);
    } else if(strcmp(req->method, "OPTIONS") == 0) {
        *response_code = handle_options(req->client_fd);
    } else {
        method_not_allowed_error(req->client_fd);
    }
}


int route_get(int client_fd, const char* path, int send_body) {
    for (int i = 0; i < get_routes_num; i++) {
        if (strcmp(path, get_routes[i].path) == 0) {
            return get_routes[i].handler(client_fd);
        }
    }

    if(strstr(path, "..")) {
        return forbidden_error(client_fd);
    }

    if(strcmp(path, "/") == 0) {
        path = "/index.html";
    }

    return serve_static_file(client_fd, path, send_body);
}

int serve_static_file(int client_fd, const char* path, int send_body) {
    char file_path[1024];
    snprintf(file_path, sizeof(file_path), "./static%s", path);

    size_t file_size;
    char *file_content = read_file(file_path, &file_size);
    const char* content_type = get_mime_type(path);

    if(!file_content) {
        return not_found_error(client_fd);
    }

    snprintf(http_response, sizeof(http_response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %lu\r\n"
        "\r\n",
        content_type,
        file_size);
    send(client_fd, http_response, strlen(http_response), 0);
    if(send_body) {
        send(client_fd, file_content, file_size, 0);
    }
    free(file_content);
    return 200;
}
