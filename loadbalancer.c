#include "loadbalancer.h"
#include "backend.h" 
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
// include poll 
#include <poll.h>

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

struct backend * select_backend(struct load_balancer * lb) {
    struct backend * backend = &lb->pool.backends[lb->current_backend];
    lb->current_backend = (lb->current_backend + 1) % lb->pool.num_backends;
    printf("Selected backend bro: %s:%d\n", backend->host, backend->port);
    return backend;
}

void init_loadbalancer(struct load_balancer * lb) {
    init_backend_pool(&lb->pool);
    lb->current_backend = 0;
    init_socket(lb);
}

void run_loadbalancer(struct load_balancer * lb) {
    while (1) {
        printf("Waiting for client connections...\n");
        int client = accept(lb->listen_fd, NULL, NULL);
        printf("Accepted client connection: %d\n", client);
        struct backend * backend = select_backend(lb);
        int backend_fd = connect_backend(backend);
        if (backend_fd < 0) {
            close(client);
  
        close(client);
    }
}