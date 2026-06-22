#include "backend.h"
#include "backend_pool.h" 

#define LISTEN_PORT 8080
#define REGISTER_PORT 7070
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <poll.h>
#include "session_table.h"
#ifndef LOADBALANCER_H
#define LOADBALANCER_H
#define MAX_CONNECTIONS 1024

struct listener {
    int fd;
    struct pollfd pollfd;
    struct sockaddr_in addr;
};

struct load_balancer {
    struct backend_pool pool;
    struct session_table session_table;
    struct listener client_listener;
    struct listener registration_listener;
    int current_backend; // for round-robin
    
};

void init_loadbalancer(struct load_balancer * lb);

void run_loadbalancer(struct load_balancer * lb);

struct backend * select_backend(struct load_balancer * lb);

#endif