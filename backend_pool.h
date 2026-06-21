#ifndef BACKEND_POOL_H
#define BACKEND_POOL_H
#include "backend.h"
struct backend_pool {
    struct backend backends[3];
    int num_backends; 
    
};

void init_backend_pool(struct backend_pool * pool);

#endif