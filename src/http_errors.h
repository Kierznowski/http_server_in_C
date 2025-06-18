#ifndef HTTP_ERRORS_H
#define HTTP_ERRORS_H
#include "config.h"

int forbidden_error(int client_fd);
int not_found_error(int client_fd);
int bad_request_error(int client_fd);
int length_required_error(int client_fd);
int internal_server_error(int client_fd);
int method_not_allowed_error(int client_fd);

#endif
