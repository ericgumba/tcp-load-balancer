#ifndef CONNECTION_H
#define CONNECTION_H
struct connection {
    int client_fd;
    int backend_fd;
};

void handle_connection(struct connection * conn);

#endif