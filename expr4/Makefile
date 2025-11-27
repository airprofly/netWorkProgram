# Makefile - 无连接TIME服务器编译脚本
#
# 使用方法：
#   make all     - 编译所有程序
#   make server  - 只编译服务器
#   make client  - 只编译客户端
#   make clean   - 清理编译文件
#   make help    - 显示帮助信息

# 编译器设置
CC = gcc
CFLAGS = -Wall -Wextra -g -O2

# 目标文件
SERVER = time_server
CLIENT = time_client

# 源文件
SERVER_SRC = time_server.c
CLIENT_SRC = time_client.c

# 头文件
HEADERS = common.h

# 默认目标：编译所有程序
.PHONY: all
all: $(SERVER) $(CLIENT)
	@echo ""
	@echo "=========================================="
	@echo "  编译完成！"
	@echo "=========================================="
	@echo "  服务器程序: $(SERVER)"
	@echo "  客户端程序: $(CLIENT)"
	@echo ""
	@echo "  运行方法:"
	@echo "    启动服务器: ./$(SERVER) [port]"
	@echo "    运行客户端: ./$(CLIENT) <server_ip> [port]"
	@echo "=========================================="

# 编译服务器
$(SERVER): $(SERVER_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(SERVER_SRC)
	@echo "Compiled: $@"

# 编译客户端
$(CLIENT): $(CLIENT_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(CLIENT_SRC)
	@echo "Compiled: $@"

# 只编译服务器
.PHONY: server
server: $(SERVER)

# 只编译客户端
.PHONY: client
client: $(CLIENT)

# 清理编译文件
.PHONY: clean
clean:
	rm -f $(SERVER) $(CLIENT)
	rm -f *.o
	@echo "Cleaned up build files."

# 显示帮助信息
.PHONY: help
help:
	@echo "=========================================="
	@echo "  无连接TIME服务器 - Makefile 帮助"
	@echo "=========================================="
	@echo ""
	@echo "  可用目标:"
	@echo "    make all     - 编译所有程序（默认）"
	@echo "    make server  - 只编译服务器"
	@echo "    make client  - 只编译客户端"
	@echo "    make clean   - 清理编译文件"
	@echo "    make help    - 显示此帮助信息"
	@echo ""
	@echo "  运行示例:"
	@echo "    ./time_server 8037"
	@echo "    ./time_client 127.0.0.1 8037"
	@echo ""
