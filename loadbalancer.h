#include "backend.h"
#define LISTEN_PORT 8080
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/wait.h>


struct load_balancer {
    struct backend backends[3];
    int listen_fd;
    int num_backends;
    struct sockaddr_in addr;
    
};

void init_loadbalancer(struct load_balancer * lb);

void run_loadbalancer(struct load_balancer * lb);