#include "request.h"

#include <stdio.h>
#include <time.h>
#include <string.h>


void parse_request(const char *buffer, char *method, char *path, char *version) {
    sscanf(buffer, "%7s %1023s %15s", method, path, version);
}

void log_request(const char* ip, const char* method, const char* path, const char* version, int status_code, const char* user_agent) {
    time_t now = time(NULL);
    struct tm* time_info = localtime(&now);

    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", time_info);

    printf("[%s] %s \"%s %s %s\" %d \"%s\"\n",
        time_str, ip, method, path, version, status_code, user_agent);
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
