#ifndef METRICS_SERVER_H
#define METRICS_SERVER_H
#include "listener.h"
struct http_req {
    char method[16];
    char path[256];
    char version[16];
};

enum parse_req_result {
    READ_FAIL,
    PARSE_FAIL,
    SUCCESS
};
struct metrics_server {
    struct listener metrics_listener;

};

enum parse_req_result parse_req(int fd, struct http_req * req);
void send_metrics(struct metrics_server * ms, char * body);
#endif