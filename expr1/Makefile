# TCP ECHO 客户端/服务器 Makefile
# 
# 使用方法:
#   make          - 编译所有程序
#   make client   - 仅编译客户端
#   make server   - 仅编译服务器
#   make clean    - 清理编译产物

CC = gcc
CFLAGS = -Wall -Wextra -O2

# 目标程序
CLIENT = echo_client
SERVER = echo_server

# 默认目标：编译所有程序
all: $(CLIENT) $(SERVER)

# 编译客户端
$(CLIENT): echo_client.c
	$(CC) $(CFLAGS) -o $@ $<
	@echo "[完成] 客户端编译成功: $(CLIENT)"

# 编译服务器
$(SERVER): echo_server.c
	$(CC) $(CFLAGS) -o $@ $<
	@echo "[完成] 服务器编译成功: $(SERVER)"

# 清理
clean:
	rm -f $(CLIENT) $(SERVER)
	@echo "[完成] 清理完成"

# 帮助信息
help:
	@echo "可用目标:"
	@echo "  make          - 编译所有程序"
	@echo "  make client   - 仅编译客户端"
	@echo "  make server   - 仅编译服务器"
	@echo "  make clean    - 清理编译产物"
	@echo ""
	@echo "运行方式:"
	@echo "  ./echo_server [端口]              - 启动服务器"
	@echo "  ./echo_client <服务器IP> [端口]   - 启动客户端"

.PHONY: all clean help
