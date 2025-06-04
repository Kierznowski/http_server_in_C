#include "request.h"

#include <stdio.h>


void parse_request(const char *buffer, char *method, char *path, char *version) {
    sscanf(buffer, "%7s %1023s %15s", method, path, version);
}

void log_request(const char* method, const char* path, const char* version) {
    printf("--- Parsed Request ---\n");
    printf("Method: %s\n", method);
    printf("Path: %s\n", path);
    printf("Version: %s\n", version);
}
