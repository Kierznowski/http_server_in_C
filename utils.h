#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

char* read_file(const char* path, size_t* out_size);
const char* get_mime_type(const char* path);

#endif