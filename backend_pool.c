#include "backend_pool.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

void init_backend_pool(struct backend_pool *pool) {
    pool->backends[0] = (struct backend){"localhost", 9001, 1, 0};
    pool->backends[1] = (struct backend){"localhost", 9002, 1, 0};
    pool->backends[2] = (struct backend){"localhost", 9003, 1, 0};
    pool->num_backends = 3;
}

bool all_unhealthy(struct backend_pool * pool) {
    bool ret = true;
    for (int i = 0; i < pool->num_backends; i++) {
        if(pool->backends[i].healthy) {
            ret = false;
            break;
        }
    }
    return ret;
}

bool contains(struct backend_pool * pool, char * host, int port) {
    for (int i = 0; i < pool->num_backends; i++) {
        if (strcmp(pool->backends[i].host, host) == 0 && pool->backends[i].port == port) {
            return true;
        }
    }
    return false;
}
void register_backend(struct backend_pool * pool, char * host, int port) {

    if (contains(pool, host, port)) {
        printf("Backend already registered");
        return;
    }
    struct backend *b =
    &pool->backends[pool->num_backends++];

    strncpy(b->host, host, sizeof(b->host) - 1);
    b->host[sizeof(b->host) - 1] = '\0';

    b->port = port;
    b->healthy = 1;
    b->connections = 0;
}