#ifndef HTTP_H
#define HTTP_H

void handle_get(int client_fd, const char* path, int send_body);
void handle_post(int client_fd, const char* buffer, int received_len);
void handle_options(int client_fd);

#endif