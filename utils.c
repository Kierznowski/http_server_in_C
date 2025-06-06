#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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

// decoding url (%xx)
void urldecode(char *dst, const char *src) {
    char a, b;
    while (*src) {
        if((*src == '%') && ((a = src[1]) && (b = src[2]))
            && (isdigit(a) && isdigit(b))) {
            
            if(a >= 'a') a -= 'a' - 'A';
            if(a >= 'A') a -= ('A' - 10); else a -= '0';
            if(b >= 'a') b -= 'a' - 'A';
            if(b >= 'A') b -= ('A' - 10); else b -= '0';

            *dst++ = 16 * a + b;
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

void parse_form_data(const char *body) {
    char *data = strdup(body);
    char *token = strtok(data, "&");
    while(token) {
        char *eq = strchr(token, '=');
        if(eq) {
            *eq = '\0';
            char decoded_key[1024];
            char decoded_val[1024];
            urldecode(decoded_key, token);
            urldecode(decoded_val, eq + 1);
            printf("Parsed: '%s' = '%s'\n", decoded_key, decoded_val);
        }
        token = strtok(NULL, "&");
    }
    free(data);
}
