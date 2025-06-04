#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* read_file(const char* path, size_t* out_size) {
    FILE* file = fopen(path, "rb");
    if(!file) return NULL;

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file);

    char* buffer = malloc(size + 1);
    if(!buffer) {
        fclose(file);
        return NULL;
    }

    fread(buffer, 1, size, file);
    buffer[size] = '\0';
    fclose(file);

    *out_size = size;
    return buffer;
}

const char* get_mime_type(const char* path) {
    const char* ext = strchr(path, '.');
    if(!ext) return "application/octet-stream"; // binary file

    ext++;
    if(strcmp(ext, "html") == 0 || strcmp(ext, "htm") == 0) return "text/html";
    if(strcmp(ext, "css") == 0) return "text/css";
    if(strcmp(ext, "js") == 0) return "application/javascript";
    if(strcmp(ext, "json") == 0) return "application/json";
    if(strcmp(ext, "png") == 0) return "image/png";
    if(strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0) return "image/jpeg";
    if(strcmp(ext, "gif") == 0) return "image/gif";
    if(strcmp(ext, "svg") == 0) return "image/svg+xml";
    if(strcmp(ext, "txt") == 0) return "text/plain";
    if(strcmp(ext, "pdf") == 0) return "application/pdf";
    if(strcmp(ext, "ico") == 0) return "image/x-icon";

    return "application/octet-stream"; // default binary
}