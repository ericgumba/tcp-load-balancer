#include "backend.h"
#include "backend_pool.h"
#define LISTEN_PORT 8080
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/wait.h>


struct load_balancer {
    struct backend_pool pool;
    int listen_fd; 
    int current_backend; // for round-robin
    struct sockaddr_in addr;
    
};

void init_loadbalancer(struct load_balancer * lb);

void run_loadbalancer(struct load_balancer * lb);