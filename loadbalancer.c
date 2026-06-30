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
struct http_req {
    char method[16];
    char path[256];
    char version[16];
};

void init_socket(struct listener * l, int port) {
    l->fd = socket(AF_INET, SOCK_STREAM, 0);
    printf("%d \n", l->fd);
    int opt = 1;
    setsockopt(l->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    l->addr.sin_family = AF_INET;
    l->addr.sin_port = htons(port);
    l->addr.sin_addr.s_addr = INADDR_ANY;
    bind(l->fd, (struct sockaddr *)&l->addr, sizeof(l->addr));
    listen(l->fd, 128);
}
 
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

enum parse_req_result {
    READ_FAIL,
    PARSE_FAIL,
    SUCCESS
};

enum parse_req_result parse_req(int fd, struct http_req * req) {

    char buf[2057];
    ssize_t n = read(fd, buf, sizeof(buf)-1);
    if (n < 0) {
        return READ_FAIL;
    }

    buf[n] = '\0';

    int parsed = sscanf(buf, "%15s %255s %15s", req->method, req->path, req->version);
    if (parsed != 3) return PARSE_FAIL;
    return SUCCESS;
}

void send_metrics(struct load_balancer * lb) {
    printf("Sending metrics \n");
    int client_fd = accept(lb->metrics_listener.fd,NULL,NULL);

    struct http_req req = {0};
    enum parse_req_result prr = parse_req(client_fd, &req);

    if (prr == READ_FAIL) {
        perror("send_metrics: unable to read from client\n");
        close(client_fd);
        return;
    }

    if (prr == PARSE_FAIL) {
        fprintf(stderr, "send_metrics: Unable to parse http request\n");
        close(client_fd);
        return;
    }


    if(strcmp(req.method, "GET") == 0 && strcmp(req.path, "/metrics") == 0) {
        char body[2056];
        int off = 0;
        off += snprintf(body + off, sizeof(body) - off,
            "active_connections %d\n"
            "registered_backends %d\n",
            lb->session_table.num_connections,
            lb->pool.num_backends);
        for (int i = 0; i < lb->pool.num_backends && off < (int)sizeof(body); i++) {
            struct backend *backend = &lb->pool.backends[i];
            off += snprintf(body + off, sizeof(body) - off,
                "backend{host=\"%s\",port=\"%d\"} healthy=%d connections=%d\n",
                backend->host,
                backend->port,
                backend->healthy,
                backend->connections);
        }
        dprintf(client_fd,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %d\r\n"
            "\r\n"
            "%s",
            off,
            body);
    }
    close(client_fd);
}

void init_loadbalancer(struct load_balancer * lb) {  
    lb->current_backend = 0;
    init_socket(&lb->client_listener, LISTEN_PORT);
    init_socket(&lb->registration_listener, REGISTER_PORT);
    init_socket(&lb->metrics_listener, METRICS_PORT);
}

int init_pollfd(struct load_balancer * lb, struct pollfd * fds) {
    int nfds = POLLFD_SESSION_STARTING_IDX + lb->session_table.num_connections * 2;
    fds[POLLFD_LISTEN_IDX].fd = lb->client_listener.fd;
    fds[POLLFD_LISTEN_IDX].events = POLLIN;
    fds[POLLFD_REGISTRATION_IDX].fd = lb->registration_listener.fd;
    fds[POLLFD_REGISTRATION_IDX].events = POLLIN;
    fds[POLLFD_METRICS_IDX].fd = lb->metrics_listener.fd;
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

    int client_fd = accept(lb->client_listener.fd, NULL, NULL);
    if (lb->pool.num_backends == 0) {
        printf("No backends available\n");
        close(client_fd);
        return;
    }
    int backend_fd = -1;
    for (int attempts = 0; attempts < lb->pool.num_backends; attempts++) {
        struct backend * backend = select_backend(lb);
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
        add_session(&lb->session_table, conn);
    } else {
        close(client_fd);
    }
}

void handle_backend_connection(struct load_balancer * lb) {

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

void run_loadbalancer(struct load_balancer * lb) {
    while (1) {
        printf("NEW LOOP \n");
        struct pollfd fds[POLLFD_SESSION_STARTING_IDX + lb->session_table.num_connections * 2];
        int nfds = init_pollfd(lb, fds);
        int n = poll(fds, nfds, -1);

        // copy back revents back into session table
        copy_revents_into(&lb->session_table, fds, POLLFD_SESSION_STARTING_IDX);

        if (n <= 0) {
            perror("poll");
            continue;
        } 

        if (fds[POLLFD_LISTEN_IDX].revents & POLLIN) {
            handle_client_connection(lb);
        }

        if (fds[POLLFD_REGISTRATION_IDX].revents & POLLIN) {
            handle_backend_connection(lb);
        }
        if (fds[POLLFD_METRICS_IDX].revents & POLLIN) {
            send_metrics(lb);
        }
        process_ready_sessions(&lb->session_table);
    }
        
}
