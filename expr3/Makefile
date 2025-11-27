CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2

all: time_client time_server

time_client: time_client.c
	$(CC) $(CFLAGS) -o time_client time_client.c

time_server: time_server.c
	$(CC) $(CFLAGS) -o time_server time_server.c

clean:
	rm -f time_client time_server
