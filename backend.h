#ifndef BACKEND_H
#define BACKEND_H

struct backend {
    char host[64];
    int port;
    int healthy;
    int connections;
};

int connect_backend(struct backend * backend); 

#endif
// {loadbalancer}
// accepts connections from clients and forwards them to backends
// has a list of backends and does health checks on them
// forwards connections to backends using round-robin or least-connections
// {backends}
// {clients}

// {server} <-> {lb} <-> {clients}
// {server} <-|
// {server} <-|
