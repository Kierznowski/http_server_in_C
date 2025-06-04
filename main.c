#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h> // files size
#include <ctype.h> // is digit function

#define PORT 8080
#define BACKLOG 10
#define BUFFER_SIZE 4096

int setup_server_socket();
void parse_request(const char *buffer, char *method, char *path, char *version);
void* handle_client_req(void* args);
void log_request(const char* method, const char* path, const char* version);
char* read_file(const char* path, size_t* out_size);
void handle_get(int client_fd, const char* path, int send_body);
void handle_post(int client_fd, const char* buffer, int received_len);
const char* get_mime_type(const char* path);
void handle_options(int client_fd);



typedef struct {
    int client_fd;
    struct sockaddr_in client_addr;
} client_info_t;


int main() {
    int server_fd = setup_server_socket(); 

    // File descriptors for server and client addreses
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd;

    printf("Server is listening on port %d...\n", PORT);

    while (1)
    {
        printf("Waiting for a new connection...\n");

        // Accepting a client. Creating socket for the client (client_fd)
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0)
        {
            perror("accept failed");
            continue;
        }

        client_info_t *client_info = malloc(sizeof(client_info_t));
        if(!client_info) {
            perror("malloc failed");
            close(client_fd);
            continue;
        }

        client_info->client_fd = client_fd;
        client_info->client_addr = client_addr;

        //handling request asynchronously
        pthread_t thread_id;
        if(pthread_create(&thread_id, NULL, handle_client_req, client_info) != 0) {
            perror("pthread_create failed");
            close(client_fd);
            free(client_info);
        } else {
            pthread_detach(thread_id); // cleanup thread after it ends
        }
    }

    close(server_fd);
    return 0;
}

int setup_server_socket() {
    // Creating TCP Socket: AF_INET - IPv4 address (alt. AF_INET6), SOCK_STREAM - TCP stream (alt. SOCK_DGRAM)
    // 0 - default protocol 0 = IPPROTO_TCP
    // On success returns socket file descriptor, on failure -1 and error
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;

    // Binding socket to port
    memset(&server_addr, 0, sizeof(server_addr)); // clear server_addr structure
    server_addr.sin_family = AF_INET;             // IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY;     // INADDR_ANY=0.0.0.0, listen on all interfaces (Ethernet, WiFi, localhost etc.)
    server_addr.sin_port = htons(PORT);           // converting to network byte order (Big Endian)
    
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Listening for connections. Backlog - number/queue of connections that can wait for calling accept()
    // listen() mark our socket as a server and set queue size. As before: ret < 0 = error
    if (listen(server_fd, BACKLOG) < 0)
    {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    return server_fd;
}

void* handle_client_req(void* arg) {
    // parse client_fd and client_addr
    client_info_t *client_info = (client_info_t*)arg;
    int client_fd = client_info->client_fd;
    struct sockaddr_in client_addr = client_info->client_addr;
    free(client_info);

    printf("Connection accepted from %s:%d\n",
            inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // copy received data into buffer
    char buffer[BUFFER_SIZE];
    char http_response[BUFFER_SIZE];
    int bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0); 
    if(bytes_received < 0) {
        perror("recv failed");
        close(client_fd);
        return NULL;
    }
    buffer[bytes_received] = '\0';
    
    // parse data from buffer
    char method[8], path[1024], version[16];
    parse_request(buffer, method, path, version);
    log_request(method, path, version);

    if(strcmp(method, "GET") == 0) {
        handle_get(client_fd, path, 1);
    } else if (strcmp(method, "HEAD") == 0){
        handle_get(client_fd, path, 0);
    } else if(strcmp(method, "POST") == 0) {
        handle_post(client_fd, buffer, bytes_received);
    } else if(strcmp(method, "OPTIONS") == 0) {
        handle_options(client_fd);
    } else {
        const char *body = "405 Method Not Allowed";
        snprintf(http_response, sizeof(http_response),
            "HTTP/1.1 405 Method Not Allowed\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %lu\r\n"
            "Allow: GET, POST\r\n"
            "\r\n"
            "%s",
            strlen(body),
            body
        );
        send(client_fd, http_response, strlen(http_response), 0);
    }


    close(client_fd);
    return NULL;
}

void parse_request(const char *buffer, char *method, char *path, char *version) {
    sscanf(buffer, "%7s %1023s %15s", method, path, version);
}

void log_request(const char* method, const char* path, const char* version) {
    printf("--- Parsed Request ---\n");
    printf("Method: %s\n", method);
    printf("Path: %s\n", path);
    printf("Version: %s\n", version);
}

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

void handle_get(int client_fd, const char* path, int send_body) {
    char http_response[BUFFER_SIZE];

    if(strstr(path, "..")) {
        const char *body = "403 Forbidden";
        snprintf(http_response, sizeof(http_response),
            "HTTP/1.1 403 Forbidden\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %lu\r\n"
            "\r\n"
            "%s",
            strlen(body),
            body
        );
        send(client_fd, http_response, strlen(http_response), 0);
        return;
    }

    if(strcmp(path, "/") == 0) {
        path = "/index.html";
    }

    char file_path[1024+16];
    snprintf(file_path, sizeof(file_path), "./static%s", path);

    size_t file_size;
    char *file_content = read_file(file_path, &file_size);
    const char* content_type = get_mime_type(path);
    
    if(!file_content) {
        const char *body = "404 Not Found";
        snprintf(http_response, sizeof(http_response),
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %lu\r\n"
            "\r\n"
            "%s",
            strlen(body),
            body
        );
        send(client_fd, http_response, strlen(http_response), 0);
    } else {
        snprintf(http_response, sizeof(http_response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %lu\r\n"
            "\r\n",
            content_type,
            file_size
        );
        send(client_fd, http_response, strlen(http_response), 0);
        if(send_body) {
            send(client_fd, file_content, file_size, 0);
        }
        free(file_content);
    }
}
    
void handle_post(int client_fd, const char* buffer, int received_len) {
    char http_response[BUFFER_SIZE];

    const char *body_start = strstr(buffer, "\r\n\r\n");
    // no body
    if (!body_start) {
        const char *body = "400 Bad Request";
        snprintf(http_response, sizeof(http_response),
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %lu\r\n"
            "\r\n"
            "%s", strlen(body), body);
        send(client_fd, http_response, strlen(http_response), 0);
        return;
    }
    // set pointer
    body_start += 4;

    int header_len = body_start - buffer;
    int body_len_in_buffer = received_len - header_len;

    const char *cl_header = strstr(buffer, "Content-Length:"); // finding content=length header
    int expected_length = 0;
    //parse expected body length
    if(cl_header) {
        cl_header += strlen("Content-Length:");
        while(*cl_header == ' ') cl_header++;

        char len_buf[16] = {0};
        int i = 0;
        while(isdigit(*cl_header) && i < (int)(sizeof(len_buf) - 1)) {
            len_buf[i++] = *cl_header++;
        }
        expected_length = atoi(len_buf); // converting char[] to int
    }

    if(expected_length <= 0) {
        const char *body = "411 Length Required";
        snprintf(http_response, sizeof(http_response),
            "HTTP/1.1 411 Length Required\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %lu\r\n"
            "\r\n"
            "%s", 
            strlen(body), body);
            send(client_fd, http_response, strlen(http_response), 0);
            return;
    }
    
    char *full_body = malloc(expected_length + 1); // +1 for \0 termination
    if(!full_body) {
        perror("malloc");
        close(client_fd);
        return;
    }

    memcpy(full_body, body_start, body_len_in_buffer);
    int total_received = body_len_in_buffer;

    // if there is remaining data
    while(total_received < expected_length) {
        int r = recv(client_fd, full_body + total_received, expected_length - total_received, 0);
        if(r <= 0) {
            perror("recv (continuation)");
            free(full_body);
            close(client_fd);
            return;
        }
        total_received += r;
    }

    full_body[expected_length] = '\0';

    printf("POST body (%d bytes):\n%s\n", expected_length, full_body);

    const char *response = "POST data received\n";
    snprintf(http_response, sizeof(http_response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %lu\r\n"
        "\r\n"
        "%s", strlen(response), response);
    send(client_fd, http_response, strlen(http_response), 0);
    
    free(full_body);
}

// get media type
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

void handle_options(int client_fd) {
    const char* allowed_methods = "GET, POST, HEAD, OPTIONS";
    char response[BUFFER_SIZE];

    snprintf(response, sizeof(response),
        "HTTP/1.1 204 No Content\r\n"
        "Allow: %s\r\n"
        "Content-Length: 0\r\n"
        "\r\n",
        allowed_methods);

    send(client_fd, response, strlen(response), 0);
}