CC=gcc
CFLAGS=-Wall -Wextra -O2

all: lb echo_server

lb: main.c
	$(CC) $(CFLAGS) -o lb main.c

echo_server: echo_server.c
	$(CC) $(CFLAGS) -o echo_server echo_server.c

clean:
	rm -f lb echo_server
