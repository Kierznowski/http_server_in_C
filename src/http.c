#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/socket.h>

#include "http.h"
#include "utils.h"
#include "router.h"
#include "config.h"

int handle_get(int client_fd, const char* path, int send_body) {
    for (int i = 0; i < num_routes; i++) {
        if (strcmp(path, routes[i].path) == 0) {
            return routes[i].handler(client_fd);
        }
    }
    
    char http_response[BUFFER_SIZE];

    if(strstr(path, "..")) {
        const char *body = "403 Forbidden";
        snprintf(http_response, sizeof(http_response),
            "HTTP/1.1 403 Forbidden\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %lu\r\n"
            "\r\n%s",
            strlen(body), body);
        send(client_fd, http_response, strlen(http_response), 0);
        return 403;
    }

    if(strcmp(path, "/") == 0) {
        path = "/index.html";
    }


    char file_path[1024];
    snprintf(file_path, sizeof(file_path), "./static%s", path);

    size_t file_size;
    char *file_content = read_file(file_path, &file_size);
    const char* content_type = get_mime_type(path);
    
    if(!file_content) {
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
    
int handle_post(int client_fd, const char* buffer, int received_len) {
    char http_response[BUFFER_SIZE];

    const char *body_start = strstr(buffer, "\r\n\r\n");
    if (!body_start) {
        const char *body = "400 Bad Request";
        snprintf(http_response, sizeof(http_response),
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %lu\r\n"
            "\r\n%s", strlen(body), body);
        send(client_fd, http_response, strlen(http_response), 0);
        return 400;
    }
    body_start += 4; // set pointer

    int header_len = body_start - buffer;
    int body_len_in_buffer = received_len - header_len;

    const char *cl_header = strstr(buffer, "Content-Length:"); // finding content=length header
    int expected_length = 0;
    //parse expected body length
    if(cl_header) {
        cl_header += strlen("Content-Length:");
        while(*cl_header == ' ') cl_header++;

        char len_buf[16] = {0};
        int i = 0;
        while(isdigit(*cl_header) && i < (int)(sizeof(len_buf) - 1)) {
            len_buf[i++] = *cl_header++;
        }
        expected_length = atoi(len_buf); // converting char[] to int
    } else {
        const char *body = "411 Length Required";
        snprintf(http_response, sizeof(http_response),
            "HTTP/1.1 411 Length Required\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %lu\r\n"
            "\r\n%s", strlen(body), body);
        send(client_fd, http_response, strlen(http_response), 0);
        return 411;
    }

    char *full_body = malloc(expected_length + 1); // +1 for \0 termination
    if(!full_body) {
        perror("malloc");
        close(client_fd);
        return 500;
    }

    memcpy(full_body, body_start, body_len_in_buffer);
    int total_received = body_len_in_buffer;

    // if there is remaining data
    while(total_received < expected_length) {
        int r = recv(client_fd, full_body + total_received, expected_length - total_received, 0);
        if(r <= 0) {
            perror("recv (continuation)");
            free(full_body);
            close(client_fd);
            return 500;
        }
        total_received += r;
    }

    full_body[expected_length] = '\0';

    printf("POST body (%d bytes):\n%s\n", expected_length, full_body);

    const char *ct_header = strstr(buffer, "Content-Type:");
    if(ct_header && strstr(ct_header, "application/x-www-form-urlencoded")) {
        parse_form_data(full_body);
    }

    const char *response = "POST data received\n";
    snprintf(http_response, sizeof(http_response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %lu\r\n"
        "\r\n"
        "%s", strlen(response), response);
    send(client_fd, http_response, strlen(http_response), 0);
    
    free(full_body);
    return 200;
}

int handle_options(int client_fd) {
    const char* allowed_methods = "GET, POST, HEAD, OPTIONS";
    char response[BUFFER_SIZE];

    snprintf(response, sizeof(response),
        "HTTP/1.1 204 No Content\r\n"
        "Allow: %s\r\n"
        "Content-Length: 0\r\n"
        "\r\n",
        allowed_methods);

    send(client_fd, response, strlen(response), 0);
    return 204;
}
