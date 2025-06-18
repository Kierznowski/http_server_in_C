#include "http_errors.h"
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

int forbidden_error(int client_fd) {
    const char *body = "403 Forbidden";
    snprintf(http_response, strlen(http_response),
        "HTTP/1.1 403 Forbidden\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %lu\r\n"
        "\r\n%s",
        strlen(body), body);
    send(client_fd, http_response, strlen(http_response), 0);
    return 403;
}

int not_found_error(int client_fd) {
    const char *body = "404 Not Found";
    snprintf(http_response, sizeof(http_response),
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %lu\r\n"
        "\r\n%s",
        strlen(body),
        body
    );
    send(client_fd, http_response, strlen(http_response), 0);
    return 404;
}

int length_required_error(int client_fd) {
    const char *body = "411 Length Required";
    snprintf(http_response, sizeof(http_response),
        "HTTP/1.1 411 Length Required\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %lu\r\n"
        "\r\n%s", strlen(body), body);
    send(client_fd, http_response, strlen(http_response), 0);
    return 411;
}

int bad_request_error(int client_fd) {
    const char *body = "400 Bad Request";
    snprintf(http_response, sizeof(http_response),
        "HTTP/1.1 400 Bad Request\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %lu\r\n"
        "\r\n%s", strlen(body), body);
    send(client_fd, http_response, strlen(http_response), 0);
    return 400;
}

int internal_server_error(int client_fd) {
    const char *body = "500 Internal Server Error";
    snprintf(http_response, sizeof(http_response),
    "HTTP/1.1 500 Internal Server Error\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: %lu\r\n"
    "\r\n%s", strlen(body), body);
    send(client_fd, http_response, strlen(http_response), 0);
    return 500;
}

int method_not_allowed_error(int client_fd) {
    const char *body = "405 Method Not Allowed";
    snprintf(http_response, sizeof(http_response),
        "HTTP/1.1 405 Method Not Allowed\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %lu\r\n"
        "Allow: GET, POST\r\n"
        "\r\n"
        "%s",
        strlen(body),
        body
    );
    send(client_fd, http_response, strlen(http_response), 0);
    return 405;
}