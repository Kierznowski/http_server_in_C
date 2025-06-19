#include "config.h"
#include "request.h"
#include "router.h"

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h> // close()

void parse_request(const char* buffer, char* method, char* path, char* version);
void log_request(const char* ip, const char* method, const char* path, const char* version, int status_code, const char* user_agent);
const char* get_header_value(const char* headers, const char* key);
void save_data_to_buffer(const int client_fd, char* buffer);

int bytes_received;

void* handle_client_req(void* arg) {
    struct request_t* req = malloc(sizeof(struct request_t));

    // parse client_fd and client_addr
    client_info_t *client_info = (client_info_t*)arg;
    int client_fd = client_info->client_fd;
    struct sockaddr_in client_addr = client_info->client_addr;
    free(client_info);

    printf("Connection accepted from %s:%d\n",
            inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    char buffer[BUFFER_SIZE];
    save_data_to_buffer(client_fd, buffer);

    // parse data from buffer
    char method[8], path[1024], version[16];
    parse_request(buffer, method, path, version);
    req->client_fd = client_fd;
    req->method = method;
    req->path = path;
    req->buffer = buffer;
    req->bytes_received = bytes_received;

    int response_code;
    route(req, &response_code);
    //handle_req_by_method(client_fd, buffer, method, path, &response_code);

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    const char* user_agent = get_header_value(buffer, "User-Agent");
    log_request(client_ip, method, path, version, response_code, user_agent);

    close(client_fd);
    return NULL;
}

void save_data_to_buffer(const int client_fd, char *buffer) {
    bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
    if(bytes_received < 0) {
        perror("recv failed");
        close(client_fd);
        return;
    }
    buffer[bytes_received] = '\0';
}

void parse_request(const char *buffer, char *method, char *path, char *version) {
    sscanf(buffer, "%7s %1023s %15s", method, path, version);
}

const char* get_header_value(const char* headers, const char* key) {
    static char value[1024];
    value[0] = '\0';

    const char* pos = strstr(headers, key);
    if(!pos) return "";

    pos += strlen(key);
    while(*pos == ':' || *pos == ' ') pos++;

    const char* end = strstr(pos, "\r\n");
    if(!end) return "";

    size_t len = end - pos;
    if(len >= sizeof(value)) len = sizeof(value) - 1;

    strncpy(value, pos, len);
    value[len] = '\0';
    return value;
}

void log_request(const char* ip, const char* method, const char* path, const char* version, int status_code, const char* user_agent) {
    time_t now = time(NULL);
    struct tm* time_info = localtime(&now);

    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", time_info);

    printf("[%s] %s \"%s %s %s\" %d \"%s\"\n",
        time_str, ip, method, path, version, status_code, user_agent);
}