# 并发ECHO服务器 Makefile

CC = gcc
CFLAGS = -Wall -Wextra -g
TARGETS = echo_server echo_client

.PHONY: all clean

all: $(TARGETS)

echo_server: echo_server.c
	$(CC) $(CFLAGS) -o $@ $<

echo_client: echo_client.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)

# 运行服务器（默认端口8888）
run_server: echo_server
	./echo_server

# 运行客户端（连接本地服务器）
run_client: echo_client
	./echo_client
