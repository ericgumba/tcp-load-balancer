#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define LISTEN_PORT 8080
#define BACKEND_PORT 9001
#define BACKEND_HOST "127.0.0.1"


int connect_backend() {
    int backend_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(BACKEND_PORT);
    inet_pton(AF_INET, BACKEND_HOST, &addr.sin_addr);

    if(connect(backend_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Connect backend");
        close(backend_fd);
        return -1;

    }
    return backend_fd;
}

void pipe_data(int from, int to) {

    char buf[4096];

    while(1) {
	ssize_t n = read(from, buf, sizeof(buf));
	if (n <= 0) break;

	ssize_t written = 0;

	while(written < n) {
		ssize_t m = write(to, buf + written, n - written);
		if (m < 0) return;
		written += m;
	}

    }

    shutdown(to, SHUT_WR);
}

void handle_client(int client_fd) {
    // 
    int backend_fd = connect_backend();
    if (backend_fd < 0) {
        close(client_fd);
        return;
    }

    pid_t client_to_backend = fork();

    if (client_to_backend == 0) {
        pipe_data(client_fd, backend_fd);
        close(client_fd);
        close(backend_fd);
        exit(0);
    }
    pid_t backend_to_client = fork();

    if(backend_to_client == 0) {
        pipe_data(backend_fd, client_fd);
        close(client_fd);
        close(backend_fd);
        exit(0);
    }

    close(client_fd);
    close(backend_fd);
    waitpid(client_to_backend, NULL, 0);
    waitpid(backend_to_client, NULL, 0);
}

int main() {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(LISTEN_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(listen_fd, 128);

    printf("loadbalancer server listening on 8080\n");

    while (1) {
        int client = accept(listen_fd, NULL, NULL);
        pid_t pid = fork();
        if (pid == 0) { // newly created child process 
            close(listen_fd);
            handle_client(client);
            exit(0);
        }

        close(client);
    }



    return 0;
}


