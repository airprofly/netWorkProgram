# TCP聊天软件 Makefile

CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -pthread

# 目标文件
SERVER = server
CLIENT = client

# 源文件
SERVER_SRC = server.c
CLIENT_SRC = client.c

# 默认目标：编译所有
all: $(SERVER) $(CLIENT)

# 编译服务器
$(SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# 编译客户端
$(CLIENT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# 清理编译产物
clean:
	rm -f $(SERVER) $(CLIENT)

# 只编译服务器
build-server: $(SERVER)

# 只编译客户端
build-client: $(CLIENT)

# 运行服务器
run-server: $(SERVER)
	./$(SERVER)

# 运行客户端
run-client: $(CLIENT)
	./$(CLIENT)

# 帮助信息
help:
	@echo "可用的 make 目标:"
	@echo "  all          - 编译服务器和客户端 (默认)"
	@echo "  build-server - 只编译服务器"
	@echo "  build-client - 只编译客户端"
	@echo "  run-server   - 编译并运行服务器"
	@echo "  run-client   - 编译并运行客户端"
	@echo "  clean        - 清理编译产物"
	@echo "  help         - 显示此帮助信息"

.PHONY: all clean build-server build-client run-server run-client help
