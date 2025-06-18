#include "config.h"
#include "request.h"
#include "http.h"
#include "http_errors.h"

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h> // close()

void parse_request(const char* buffer, char* method, char* path, char* version);
void log_request(const char* ip, const char* method, const char* path, const char* version, int status_code, const char* user_agent);
const char* get_header_value(const char* headers, const char* key);
void handle_req_by_method(const char* method, char* path, int* response_code);

int client_fd;
char buffer[BUFFER_SIZE];
int bytes_received;

void* handle_client_req(void* arg) {
    // parse client_fd and client_addr
    client_info_t *client_info = (client_info_t*)arg;
    client_fd = client_info->client_fd;
    struct sockaddr_in client_addr = client_info->client_addr;
    free(client_info);

    printf("Connection accepted from %s:%d\n",
            inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // copy received data into buffer
    bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
    if(bytes_received < 0) {
        perror("recv failed");
        close(client_fd);
        return NULL;
    }
    buffer[bytes_received] = '\0';

    // parse data from buffer
    char method[8], path[1024], version[16];
    parse_request(buffer, method, path, version);

    int response_code;
    handle_req_by_method(method, path, &response_code);

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    const char* user_agent = get_header_value(buffer, "User-Agent");
    log_request(client_ip, method, path, version, response_code, user_agent);

    close(client_fd);
    return NULL;
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

void handle_req_by_method(const char* method, char* path, int* response_code) {
    if(strcmp(method, "GET") == 0) {
        *response_code = handle_get(client_fd, path, 1);
    } else if (strcmp(method, "HEAD") == 0){
        *response_code = handle_get(client_fd, path, 0);
    } else if(strcmp(method, "POST") == 0) {
        *response_code = handle_post(client_fd, buffer, bytes_received);
    } else if(strcmp(method, "OPTIONS") == 0) {
        *response_code = handle_options(client_fd);
    } else {
        method_not_allowed_error(client_fd);
    }
}

void log_request(const char* ip, const char* method, const char* path, const char* version, int status_code, const char* user_agent) {
    time_t now = time(NULL);
    struct tm* time_info = localtime(&now);

    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", time_info);

    printf("[%s] %s \"%s %s %s\" %d \"%s\"\n",
        time_str, ip, method, path, version, status_code, user_agent);
}