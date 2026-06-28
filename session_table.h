#ifndef SESSION_TABLE_H
#define SESSION_TABLE_H

#include "proxy_session.h"
#include <poll.h>
#define MAX_CONNECTIONS 1024

struct session_table {
    struct proxy_session connections[MAX_CONNECTIONS]; 
    int num_connections;
};


void add_session(struct session_table * sess_table, struct proxy_session conn);
void remove_session(struct session_table * sess_table, int index);
void process_ready_sessions(struct session_table * sess_table);

void copy_revents_into(struct session_table * sess_table, struct pollfd * pollfds, int starting_index);

#endif