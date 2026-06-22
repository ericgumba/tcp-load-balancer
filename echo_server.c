#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#define MAX_CONNECTIONS 1024

void register_w_lb(int port) {
 
    int lb_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(7070);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    if(connect(lb_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Connect LB");
        close(lb_fd);
        return;
    } 

    char buf[128]; 
    snprintf(
        buf,
        sizeof(buf),
        "REGISTER 127.0.0.1 %d\n",
        port
    );
    printf("sending message %s", buf);
    write(lb_fd, buf, strlen(buf));
    close(lb_fd);


}
int main(int argc, char *argv[]) {
    int port = argc > 1 ? atoi(argv[1]) : 9001;
    // register_w_lb(port);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(fd, 128);



    struct pollfd fds[MAX_CONNECTIONS];
    fds[0].fd = fd;
    fds[0].events = POLLIN;
    int current_connections = 1;


    printf("echo server: listening on %d\n", port);

    while (1) {
        int n = poll(fds, current_connections, -1);
        if (n <= 0) {
            perror("poll");
            continue;
        }
        if (fds[0].revents & POLLIN) {
            int client = accept(fd, NULL, NULL);
            if (current_connections < MAX_CONNECTIONS) {
                fds[current_connections].fd = client;
                fds[current_connections].events = POLLIN;
                current_connections++;
            } else {
                close(client);
            }
        }
        for (int i = 1; i < current_connections; i++) {
            if (fds[i].revents & POLLIN) {
                char buf[4096];
                ssize_t n = read(fds[i].fd, buf, sizeof(buf));
                if (n <= 0) {
                    close(fds[i].fd);
                    fds[i] = fds[current_connections - 1];
                    current_connections--;
                } else {
                    printf("client %d: %.*s", fds[i].fd, (int)n, buf);
                    const char * prefix = "Echo: ";
                    char response[4096];
                    // combine prefix and buf into response
                    snprintf(response, sizeof(response), "%s%.*s", prefix, (int)n, buf);
                    write(fds[i].fd, response, strlen(response));
                }
            }
        }
    }



    return 0;
}
