CC=gcc
CFLAGS=-Wall -Wextra -O2
LBS=loadbalancer.c backend_pool.c proxy_session.c backend.c session_table.c listener.c metrics_server.c
LB_SRCS=main.c $(LBS)


all: lb echo_server test_select_backend

lb: $(LB_SRCS)
	$(CC) $(CFLAGS) -o lb $(LB_SRCS)

echo_server: echo_server.c
	$(CC) $(CFLAGS) -o echo_server echo_server.c

test_select_backend: tests/test_select_backend.c $(LBS)
	$(CC) $(CFLAGS) -o test_select_backend tests/test_select_backend.c $(LBS)

test: test_select_backend
	./test_select_backend
clean:
	rm -f lb echo_server test_select_backend
