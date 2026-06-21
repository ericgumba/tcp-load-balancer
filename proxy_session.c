#include "proxy_session.h"
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

void pipe_data(int from_fd, int to_fd) {
    char buf[4096];
    ssize_t n = read(from_fd, buf, sizeof(buf));
    if (n <= 0) {  
        return; // EOF or error
    }
    write(to_fd, buf, n); 
}

void close_connection(struct proxy_session * conn) {
    close(conn->client_fd);
    close(conn->backend_fd);
}
void session_on_client_ready(struct proxy_session * conn) {
    pipe_data(conn->client_fd, conn->backend_fd); 
}
void session_on_backend_ready(struct proxy_session * conn) {
    pipe_data(conn->backend_fd, conn->client_fd); 
}

struct proxy_session create_connection(int client_fd, int backend_fd) {
    struct proxy_session conn;
    conn.client_fd = client_fd;
    conn.backend_fd = backend_fd;
    conn.client_pollfd.fd = client_fd;
    conn.client_pollfd.events = POLLIN;
    conn.backend_pollfd.fd = backend_fd;
    conn.backend_pollfd.events = POLLIN;
    return conn;
} 