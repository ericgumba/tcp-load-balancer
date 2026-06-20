#include "loadbalancer.h"
#include "connection.h"
void init_socket(struct load_balancer * lb) {
    lb->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(lb->listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    lb->addr.sin_family = AF_INET;
    lb->addr.sin_port = htons(LISTEN_PORT);
    lb->addr.sin_addr.s_addr = INADDR_ANY;
    bind(lb->listen_fd, (struct sockaddr *)&lb->addr, sizeof(lb->addr));
    listen(lb->listen_fd, 128);
}

int connect_backend(struct load_balancer * lb) {
    // TODO: implement backend selection and connection
    int backend_index = lb->current_backend;

    lb->current_backend = (lb->current_backend + 1) % lb->pool.num_backends;
    struct backend * backend = &lb->backends[backend_index];
    int backend_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(backend->port);
    inet_pton(AF_INET, backend->host, &addr.sin_addr);
    if(connect(backend_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Connect backend");
        close(backend_fd);
        return -1;
    }
    return backend_fd;
}

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

void handle_client(struct load_balancer * lb, int client_fd) {

    int backend_fd = connect_backend(lb);
    if (backend_fd < 0) {
        close(client_fd);
        return;
    }
    struct connection conn = {client_fd, backend_fd};

    handle_connection(&conn);
    

}

////////////////////////////////////

void init_loadbalancer(struct load_balancer * lb) {
    init_backend_pool(&lb->pool);
    lb->current_backend = 0;
    init_socket(lb);
}

void run_loadbalancer(struct load_balancer * lb) {
    while (1) {
        int client = accept(lb->listen_fd, NULL, NULL);
        pid_t pid = fork();
        if (pid == 0) { // newly created child process 
            close(lb->listen_fd);
            handle_client(lb, client);
            exit(0);
        }
        close(client);
    }
}