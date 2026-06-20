#include "backend.h"
struct backend_pool {
    struct backend backends[3];
    int num_backends; 
    
};

void init_backend_pool(struct backend_pool * pool) {
    pool->backends[0] = (struct backend){"localhost", 9001, 1, 0};
    pool->backends[1] = (struct backend){"localhost", 9002, 1, 0};
    pool->backends[2] = (struct backend){"localhost", 9003, 1, 0};
    pool->num_backends = 3;
}