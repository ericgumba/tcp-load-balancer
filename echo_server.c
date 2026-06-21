#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    int port = argc > 1 ? atoi(argv[1]) : 9001;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(fd, 128);

    printf("echo server: listening on %d\n", port);

    while (1) {
        int client = accept(fd, NULL, NULL);

        char buf[4096];
        ssize_t n;

        while ((n = read(client, buf, sizeof(buf))) > 0) {
            printf("echo server received: %.*s\n", (int)n, buf);
	    write(client, buf, n);
        }
    }



    return 0;
}
