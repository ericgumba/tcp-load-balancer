
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <poll.h>

struct listener {
    int fd;
    struct pollfd pollfd;
    struct sockaddr_in addr;
};

void init_listener(struct listener * l, int port);