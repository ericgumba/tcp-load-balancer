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
    lb->listener.fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(lb->listener.fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    lb->addr.sin_family = AF_INET;
    lb->addr.sin_port = htons(LISTEN_PORT);
    lb->addr.sin_addr.s_addr = INADDR_ANY;
    bind(lb->listener.fd, (struct sockaddr *)&lb->addr, sizeof(lb->addr));
    listen(lb->listener.fd, 128);
}

struct backend * select_backend(struct load_balancer * lb) {
    struct backend * backend = &lb->pool.backends[lb->current_backend];
    lb->current_backend = (lb->current_backend + 1) % lb->pool.num_backends;
    return backend;
}

void init_loadbalancer(struct load_balancer * lb) {
    init_backend_pool(&lb->pool);
    lb->current_backend = 0;
    init_socket(lb);
}

void run_loadbalancer(struct load_balancer * lb) {
    while (1) {
        struct pollfd fds[1 + lb->session_table.num_connections * 2];
        fds[0].fd = lb->listener.fd;
        fds[0].events = POLLIN;
        for (int i = 0; i < lb->session_table.num_connections; i++) {
            fds[1 + i * 2].fd = lb->session_table.connections[i].client_pollfd.fd;
            fds[1 + i * 2].events = lb->session_table.connections[i].client_pollfd.events;
            fds[1 + i * 2 + 1].fd = lb->session_table.connections[i].backend_pollfd.fd;
            fds[1 + i * 2 + 1].events = lb->session_table.connections[i].backend_pollfd.events;
        }
        int n = poll(fds, 1 + lb->session_table.num_connections * 2, -1);

        for (int i = 0; i < lb->session_table.num_connections; i++) {
            lb->session_table.connections[i].client_pollfd.revents =
                fds[1 + i * 2].revents;

            lb->session_table.connections[i].backend_pollfd.revents =
                fds[1 + i * 2 + 1].revents;
        }

        if (n <= 0) {
            perror("poll");
            continue;
        } 
        if (fds[0].revents & POLLIN) {
            int client_fd = accept(lb->listener.fd, NULL, NULL);
            struct backend * backend = select_backend(lb);
            int backend_fd = connect_backend(backend);
            if (backend_fd >= 0) {
                struct proxy_session conn = create_connection(client_fd, backend_fd);
                add_session(&lb->session_table, conn);
            } else {
                close(client_fd);
            }
        }
        process_ready_sessions(&lb->session_table);
    }
        
}
