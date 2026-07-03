#include "loadbalancer.h"
#include "backend.h" 
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
// include poll 
#include <poll.h>
#include <stdbool.h>

#define POLLFD_LISTEN_IDX 0
#define POLLFD_REGISTRATION_IDX 1
#define POLLFD_METRICS_IDX 2
#define POLLFD_SESSION_STARTING_IDX 3

struct backend * select_backend(struct load_balancer * lb) {
    for (int attempts = 0; attempts < lb->pool.num_backends; attempts++) {
        struct backend * backend = &lb->pool.backends[lb->current_backend];
        lb->current_backend = (lb->current_backend + 1) % lb->pool.num_backends;
        if(backend->healthy) {
            return backend;
        }

    }
    return NULL;
}

int build_metrics_body(struct load_balancer *lb, char *body, size_t body_size) {
    int off = 0;

    off += snprintf(body + off, body_size - off,
        "active_connections %d\n"
        "registered_backends %d\n",
        lb->session_table.num_connections,
        lb->pool.num_backends);

    for (int i = 0; i < lb->pool.num_backends && off < (int)body_size; i++) {
        struct backend *backend = &lb->pool.backends[i];
        printf("GUMBA %d \n", backend->connections);
        off += snprintf(body + off, body_size - off,
            "backend{host=\"%s\",port=\"%d\"} healthy=%d connections=%d\n",
            backend->host,
            backend->port,
            backend->healthy,
            backend->connections);
    }

    return off;
}

void init_loadbalancer(struct load_balancer * lb) {  
    lb->current_backend = 0;
    init_listener(&lb->client_listener, LISTEN_PORT);
    init_listener(&lb->registration_listener, REGISTER_PORT);
    init_listener(&lb->ms.metrics_listener, METRICS_PORT);
}

int init_pollfd(struct load_balancer * lb, struct pollfd * fds) {
    int nfds = POLLFD_SESSION_STARTING_IDX + lb->session_table.num_connections * 2;
    fds[POLLFD_LISTEN_IDX].fd = lb->client_listener.fd;
    fds[POLLFD_LISTEN_IDX].events = POLLIN;
    fds[POLLFD_REGISTRATION_IDX].fd = lb->registration_listener.fd;
    fds[POLLFD_REGISTRATION_IDX].events = POLLIN;
    fds[POLLFD_METRICS_IDX].fd = lb->ms.metrics_listener.fd;
    fds[POLLFD_METRICS_IDX].events = POLLIN;
    for (int i = 0; i < lb->session_table.num_connections; i++) {
        fds[POLLFD_SESSION_STARTING_IDX + i * 2].fd = lb->session_table.connections[i].client_pollfd.fd;
        fds[POLLFD_SESSION_STARTING_IDX + i * 2].events = lb->session_table.connections[i].client_pollfd.events;
        fds[POLLFD_SESSION_STARTING_IDX + i * 2 + 1].fd = lb->session_table.connections[i].backend_pollfd.fd;
        fds[POLLFD_SESSION_STARTING_IDX + i * 2 + 1].events = lb->session_table.connections[i].backend_pollfd.events;
    }
    return nfds;
}

void handle_client_connection(struct load_balancer * lb) {
    if (lb->pool.num_backends == 0) {
        printf("No backends available\n");
        return;
    }

    int client_fd = accept(lb->client_listener.fd, NULL, NULL);
    int backend_fd = -1;
    struct backend * backend = NULL;
    for (int attempts = 0; attempts < lb->pool.num_backends; attempts++) {
        backend = select_backend(lb);
        if (backend == NULL) {
            printf("no healthy backends available\n");
            break;
        }

        backend_fd = connect_backend(backend); 
        if (backend_fd < 0) {
            backend->healthy = 0;
            continue;
        }
        break;
    }
    
    if (backend_fd >= 0) {
        struct proxy_session conn = create_session(client_fd, backend_fd);
        conn.backend = backend;
        add_session(&lb->session_table, conn);
    } else {
        close(client_fd);
    }
}

void handle_backend_register(struct load_balancer * lb) {

    printf("ATTEMPTING TO REG\n");
    int backend_register_fd = accept(lb->registration_listener.fd, NULL, NULL);
    char buf[4096];
    ssize_t n = read(backend_register_fd, buf, sizeof(buf));
    if (n <= 0) {  
        printf("???\n");
        close(backend_register_fd); // EOF or error
        return;
    }
    buf[n] = '\0';
    char host[64];
    int port;
    if (sscanf(buf, "REGISTER %63s %d", host, &port) == 2) {
        register_backend(&lb->pool, host, port);
    } else {
        printf("Bad registration message %s \n", buf);
    }
    close(backend_register_fd); 
}

void handle_metrics(struct load_balancer * lb) {
    char body[4096] = {0};
    build_metrics_body(lb, body, sizeof(body));
    send_metrics(&lb->ms, body);
}

void run_loadbalancer(struct load_balancer * lb) {
    while (1) {
        printf("NEW LOOP \n");
        struct pollfd fds[POLLFD_SESSION_STARTING_IDX + lb->session_table.num_connections * 2];
        int nfds = init_pollfd(lb, fds);
        int n = poll(fds, nfds, 1000);

        health_check(&lb->pool);

        // copy back revents back into session table
        copy_revents_into(&lb->session_table, fds, POLLFD_SESSION_STARTING_IDX);

        if (n <= 0) {
            continue;
        } 

        if (fds[POLLFD_LISTEN_IDX].revents & POLLIN) {
            handle_client_connection(lb);
        }

        if (fds[POLLFD_REGISTRATION_IDX].revents & POLLIN) {
            handle_backend_register(lb);
        }
        if (fds[POLLFD_METRICS_IDX].revents & POLLIN) {
            handle_metrics(lb);
        }
        process_ready_sessions(&lb->session_table);
    }
}
