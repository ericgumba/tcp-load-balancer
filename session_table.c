#include "session_table.h"
#include <stdio.h>
void add_session(struct session_table * sess_table, struct proxy_session conn) {
    printf("Adding a new session \n");
    if (sess_table->num_connections < MAX_CONNECTIONS) {
        sess_table->connections[sess_table->num_connections] = conn;
        sess_table->num_connections++;
        printf("Success\n");
    }
}

void copy_revents_into(struct session_table * sess_table, struct pollfd * pollfds, int starting_index) {
    for(int i = 0; i < sess_table->num_connections; i++) {
        sess_table->connections[i].client_pollfd.revents = pollfds[starting_index + (2 * i)].revents;
        sess_table->connections[i].backend_pollfd.revents = pollfds[starting_index + (2 * i) + 1].revents;
    }
}

void remove_session(struct session_table * sess_table, int i) {
    sess_table->connections[i] = sess_table->connections[sess_table->num_connections];
    sess_table->num_connections--;
}

void process_ready_sessions(struct session_table * sess_table) {
    for(int i = 0; i < sess_table->num_connections; i++) {
        struct proxy_session * conn = &sess_table->connections[i];
        if ( conn->client_pollfd.revents & POLLIN) {
            session_on_client_ready(conn);
        }

        if ( conn->client_pollfd.revents & (POLLERR | POLLHUP)) {
            remove_session(sess_table, i);
        }

        if ( conn->backend_pollfd.revents & (POLLERR | POLLHUP)) {
            remove_session(sess_table, i);
        }
        
        if(conn->backend_pollfd.revents & POLLIN) {
            session_on_backend_ready(conn);
        }
    }
}