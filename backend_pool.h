#ifndef BACKEND_POOL_H
#define BACKEND_POOL_H
#include "backend.h"
#include <stdbool.h>

struct backend_pool {
    struct backend backends[3];
    int num_backends; 
    
};

bool all_unhealthy(struct backend_pool * pool);
void init_backend_pool(struct backend_pool * pool);
void health_check(struct backend_pool * pool);
void register_backend(struct backend_pool * pool, char * host, int port);
#endif