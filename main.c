#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include "loadbalancer.h"

#define LISTEN_PORT 8080
#define BACKEND_PORT 9001
#define BACKEND_HOST "127.0.0.1"


int main() {
    printf("Starting load balancer on port %d\n", LISTEN_PORT);
    struct load_balancer lb = {0};
    printf("INIT\n");
    init_loadbalancer(&lb);
    printf("RUN\n");
    run_loadbalancer(&lb);

    return 0;
}


 