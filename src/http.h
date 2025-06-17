#ifndef HTTP_H
#define HTTP_H

int handle_get(int client_fd, const char* path, int send_body);
int handle_post(int client_fd, const char* buffer, int received_len);
int handle_options(int client_fd);

#endif