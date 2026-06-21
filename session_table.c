#include "session_table.h"

void add_session(struct session_table * sess_table, struct proxy_session conn) {
    if (sess_table->num_connections < MAX_CONNECTIONS) {
        sess_table->connections[sess_table->num_connections++] = conn;
    }
}

void process_ready_sessions(struct session_table * sess_table) {
    for(int i = 0; i < sess_table->num_connections; i++) {
        struct proxy_session * conn = &sess_table->connections[i];
        if ( conn->client_pollfd.revents & POLLIN) {
            session_on_client_ready(conn);
        }

        if(conn->backend_pollfd.revents & POLLIN) {
            session_on_backend_ready(conn);
        }
    }
}