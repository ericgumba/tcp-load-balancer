#include "metrics_server.h"
#include "loadbalancer.h"

enum parse_req_result parse_req(int fd, struct http_req * req) {

    char buf[2057];
    ssize_t n = read(fd, buf, sizeof(buf)-1);
    if (n < 0) {
        return READ_FAIL;
    }

    buf[n] = '\0';

    int parsed = sscanf(buf, "%15s %255s %15s", req->method, req->path, req->version);
    if (parsed != 3) return PARSE_FAIL;
    return SUCCESS;
}

void send_metrics(struct metrics_server * ms, char * body) {
    printf("Sending metrics \n");
    int client_fd = accept(ms->metrics_listener.fd,NULL,NULL);

    struct http_req req = {0};
    enum parse_req_result prr = parse_req(client_fd, &req);

    if (prr == READ_FAIL) {
        perror("send_metrics: unable to read from client\n");
        close(client_fd);
        return;
    }

    if (prr == PARSE_FAIL) {
        fprintf(stderr, "send_metrics: Unable to parse http request\n");
        close(client_fd);
        return;
    }


    if(strcmp(req.method, "GET") == 0 && strcmp(req.path, "/metrics") == 0) {
        dprintf(client_fd,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %d\r\n"
            "\r\n"
            "%s",
            (int)strlen(body),
            body);
    }
    close(client_fd);
}
