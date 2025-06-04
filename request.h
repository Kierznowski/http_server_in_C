#ifdef REQUEST_H
#define REQUEST_H

void parse_request(const char *buffer, char *method, char *path, char *version);
void log_request(const char* method, const char* path, const char* version);

#endif