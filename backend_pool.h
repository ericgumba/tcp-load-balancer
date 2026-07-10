#ifndef BACKEND_POOL_H
#define BACKEND_POOL_H
#include "backend.h"
#include <stdbool.h>

#include <strategy.h>

struct backend_pool {
    enum strategy strategy;
    struct backend backends[1024]; 
    struct backend * (*select)(struct backend_pool * self);
    int num_backends;
    int rr_index;
    
};

bool all_unhealthy(struct backend_pool * pool);
void init_backend_pool(struct backend_pool * pool);
void health_check(struct backend_pool * pool);
void register_backend(struct backend_pool * pool, char * host, int port);
#endif