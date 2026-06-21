#include "connection.h"
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>


void close_connection(struct connection * conn) {
    close(conn->client_fd);
    close(conn->backend_fd);
}

void pipe_data(int from_fd, int to_fd) {
    char buf[4096];
    ssize_t n;
    while ((n = read(from_fd, buf, sizeof(buf))) > 0) {
        ssize_t written = 0;
        while (written < n) {
            ssize_t m = write(to_fd, buf + written, n - written);
            if (m < 0) return;
            written += m;
        }
    }
}

void handle_connection(struct connection * conn) {
    pid_t client_to_backend = fork();

    if (client_to_backend == 0) {
        pipe_data(conn->client_fd, conn->backend_fd);
        close_connection(conn);
        exit(0);
    }
    pid_t backend_to_client = fork();

    if(backend_to_client == 0) {
        pipe_data(conn->backend_fd, conn->client_fd);
        close_connection(conn);
        exit(0);
    }

    close_connection(conn);
    waitpid(client_to_backend, NULL, 0);
    waitpid(backend_to_client, NULL, 0);
}