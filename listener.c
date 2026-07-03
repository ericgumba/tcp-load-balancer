#include "listener.h"
void init_listener(struct listener * l, int port) {
    l->fd = socket(AF_INET, SOCK_STREAM, 0);
    printf("%d \n", l->fd);
    int opt = 1;
    setsockopt(l->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    l->addr.sin_family = AF_INET;
    l->addr.sin_port = htons(port);
    l->addr.sin_addr.s_addr = INADDR_ANY;
    bind(l->fd, (struct sockaddr *)&l->addr, sizeof(l->addr));
    listen(l->fd, 128);
}
 //