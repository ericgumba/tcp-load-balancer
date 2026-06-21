#ifndef PROXY_SESSION_H
#define PROXY_SESSION_H

#include <poll.h>
struct proxy_session {
    int client_fd;
    int backend_fd;
    struct pollfd client_pollfd;
    struct pollfd backend_pollfd;
};

void session_on_client_ready(struct proxy_session * conn);
void session_on_backend_ready(struct proxy_session * conn);
 
struct proxy_session create_connection(int client_fd, int backend_fd);
#endif