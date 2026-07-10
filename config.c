#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
struct config parse_config_file(char * file) {
    struct config ret = {0};
    FILE *f = fopen(file, "r");

    if (file == NULL) {
        perror("Error reading file"); 
        return ret;
    }

    char *line = NULL;
    size_t len = 0;

    while (getline(&line, &len, f) != -1) {
        char starting[20];
        sscanf(line, "%19s", starting);
        printf("SIGH \n");

        if (strstr(line, "backend")) {
            char * port_ptr = strstr(line, "port=");
            char * host_ptr = strstr(line, "host=");
            int port = 0;
            char host_name[40];

            if(host_ptr && port_ptr) {
                sscanf(port_ptr, "port=%d", &port);
                sscanf(host_ptr, "host=%39s", host_name);
            }
            register_backend(&ret.p, host_name, port);
        }
        if (strstr(line, "strategy=")) {
            printf("YESYES\n");
            char * strat = strstr(line, "strategy=");
            if (strat)
            if (strcmp(strat, "least_connections")){ 
                printf("least connections\n");
                ret.p.strategy = LEAST_CONNECTIONS;
            }
            // defaults to round_robin
        }

    }

    return ret;
    
}