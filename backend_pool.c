#include "backend_pool.h"
#include <string.h>
void init_backend_pool(struct backend_pool *pool) {
    pool->backends[0] = (struct backend){"localhost", 9001, 1, 0};
    pool->backends[1] = (struct backend){"localhost", 9002, 1, 0};
    pool->backends[2] = (struct backend){"localhost", 9003, 1, 0};
    pool->num_backends = 3;
}
void register_backend(struct backend_pool * pool, char * host, int port) {

    struct backend *b =
    &pool->backends[pool->num_backends++];

    strncpy(b->host, host, sizeof(b->host) - 1);
    b->host[sizeof(b->host) - 1] = '\0';

    b->port = port;
    b->healthy = 1;
    b->connections = 0;
}