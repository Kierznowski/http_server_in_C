#ifndef REQUEST_H
#define REQUEST_H

void parse_request(const char* buffer, char* method, char* path, char* version);
void log_request(const char* ip, const char* method, const char* path, const char* version, int status_code, const char* user_agent);
const char* get_header_value(const char* headers, const char* key);

#endif