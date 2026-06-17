#include "loadbalancer.h"

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

////////////////////////////////////

void init_loadbalancer(struct load_balancer * lb) {
    lb->backends[0] = (struct backend){"localhost", 9001, 1, 0};
    lb->backends[1] = (struct backend){"localhost", 9002, 1, 0};
    lb->backends[2] = (struct backend){"localhost", 9003, 1, 0};
    lb->num_backends = 3;
    init_socket(lb);
}

void run_loadbalancer(struct load_balancer * lb) {
    while (1) {
        int client = accept(lb->listen_fd, NULL, NULL);
        pid_t pid = fork();