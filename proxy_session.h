#ifndef PROXY_SESSION_H
#define PROXY_SESSION_H

#include <poll.h>
#include "backend.h"
struct proxy_session {
    int client_fd;
    int backend_fd;
    struct pollfd client_pollfd;
    struct pollfd backend_pollfd;
    struct backend * backend;
};

void session_on_client_ready(struct proxy_session * conn);
void session_on_backend_ready(struct proxy_session * conn);

struct proxy_session create_session(int client_fd, int backend_fd);
#endif