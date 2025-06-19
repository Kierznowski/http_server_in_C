#include "server.h"

#define PORT 8080
#define BACKLOG 10

int main() {
    server_start(PORT, BACKLOG);
    server_close();

    return 0;
}