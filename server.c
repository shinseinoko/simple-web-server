#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1048576 // 1 mb

void check(int syscall, const char* what) {
    if (syscall < 0) {
        fprintf(stderr, "%s\n", what);
        exit(EXIT_FAILURE);
    }
}


void parse_request(char* request, char** file_name, char** file_extension) {
    char* fn = *file_name;
    char* fe = *file_extension;
    size_t method_size = 0, file_name_size = 0, file_ext_size = 0;

    for (int i = 0; request[i] != ' '; i++)
        method_size++;
    char* method = (char*)malloc((method_size + 1) * sizeof(char));
    strncpy(method, request, method_size);
    method[method_size] = '\0';
    if (strcmp(method, "GET")) {
        fprintf(stderr, "Can't handle such method\n");
        free(method);
        exit(EXIT_FAILURE);
    }

    for (int i = strlen(method) + 2; request[i] != ' '; i++)
        file_name_size++;
    fn = (char*)malloc((file_name_size + 1) * sizeof(char));
    strncpy(fn, request + strlen(method) + 2, file_name_size);
    fn[file_name_size] = '\0';

    int i = 0, j = 0;
    for (i = 0; fn[i] != '\0' && fn[i] != '.'; i++);
    for (j = i; fn[j] != '\0'; j++)
        file_ext_size++;
    fe = (char*)malloc((file_ext_size + 1) * sizeof(char));
    strncpy(fe, fn + i + 1, file_ext_size);
    fe[file_ext_size] = '\0';
    *file_name = fn;
    *file_extension = fe;
    free(method);
}

const char* get_mime(char* file_ext) {
    if (!strcmp(file_ext, "html"))
        return "text/html";
    else if (!strcmp(file_ext, "txt"))
        return "text/plain";
    else if (!strcmp(file_ext, "jpeg") || !strcmp(file_ext, "jpg"))
        return "image/jpeg";
    else if (!strcmp(file_ext, "png"))
        return "image/png";
    else if (!strcmp(file_ext, "pdf"))
        return "application/pdf";
    else return "application/octet-stream";
}

char* create_response(char* file_name, char* file_exntesion, size_t* response_size) {
    const char* mime = get_mime(file_exntesion);
    char* response = (char*)malloc(BUFFER_SIZE * 2 * sizeof(char));
    int fd = open(file_name, O_RDONLY);
    if (fd < 0) {
        snprintf(response, BUFFER_SIZE, "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n404 Not Found");
        *response_size = strlen(response);
        return response;
    }
    struct stat filestats;
    stat(file_name, &filestats);
    int file_size = filestats.st_size;
    snprintf(response, BUFFER_SIZE, 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Size: %d\r\n"
            "\r\n", mime, file_size);
    *response_size = strlen(response);
    size_t bytes_read;
    while ((bytes_read = read(fd, response + *response_size, BUFFER_SIZE - *response_size)) > 0)
        *response_size += bytes_read;
    close(fd);
    return response; 
}

void handle_request(int server_fd, int client_fd) {
    char* request = (char*)malloc(BUFFER_SIZE * sizeof(char));
    size_t response_size;
    recv(client_fd, request, BUFFER_SIZE, 0);
    puts(request);
    char* file_name;
    char* file_extension;
    parse_request(request, &file_name, &file_extension);
    char* response = create_response(file_name, file_extension, &response_size);
    send(client_fd, response, response_size, 0);
    free(request);
    free(response);
    free(file_name);
    free(file_extension);
    close(client_fd);
}

volatile sig_atomic_t flag = 1;

void handler(int) {
    flag = 0;
}

int main() {
    int server_fd, client_fd;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    check(server_fd, "socket() failed");

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    int opt = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    check(bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)), "bind() failed");
    check(listen(server_fd, MAX_CLIENTS), "listen() failed");
    signal(SIGINT, handler);
    while (flag) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
        check(client_fd, "accept() failed");
        handle_request(server_fd, client_fd);
    }
    close(server_fd);
    return 0;
}