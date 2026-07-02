#include "backend.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>

int open_backend_socket(struct backend * backend) {
    int backend_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(backend->port);
    inet_pton(AF_INET, backend->host, &addr.sin_addr);
    return connect(backend_fd, (struct sockaddr *)&addr, sizeof(addr));
}

int connect_backend(struct backend * backend) {
    int fd = open_backend_socket(backend);

    if(fd < 0) {
        perror("Connect backend");
        close(fd);
        return -1;
    }

    backend->connections++;
    return fd;
}