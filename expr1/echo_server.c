/**
 * TCP ECHO 服务器程序
 *
 * 功能：接收客户端发送的数据，并将数据原样返回（回显）
 *
 * 编译：gcc -o echo_server echo_server.c
 * 运行：./echo_server [端口号]
 * 示例：./echo_server 7777
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/* 常量定义 */
#define BUFFER_SIZE 1024  /* 缓冲区大小 */
#define DEFAULT_PORT 7777 /* 默认监听端口（使用非特权端口便于测试） */
#define BACKLOG 5         /* 连接队列长度 */

/**
 * 打印使用说明
 */
void print_usage(const char *program_name) {
  printf("用法: %s [端口号]\n", program_name);
  printf("示例: %s 7777\n", program_name);
  printf("说明: 端口号默认为 %d\n", DEFAULT_PORT);
}

/**
 * 处理客户端连接
 */
void handle_client(int client_fd, struct sockaddr_in *client_addr) {
  char buffer[BUFFER_SIZE];
  ssize_t recv_len;
  char client_ip[INET_ADDRSTRLEN];

  /* 获取客户端 IP 地址 */
  inet_ntop(AF_INET, &client_addr->sin_addr, client_ip, INET_ADDRSTRLEN);
  printf("[信息] 客户端已连接: %s:%d\n", client_ip,
         ntohs(client_addr->sin_port));

  /* 循环接收并回显数据 */
  while ((recv_len = recv(client_fd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
    buffer[recv_len] = '\0';
    printf("[接收] 来自 %s: %s (%zd 字节)\n", client_ip, buffer, recv_len);

    /* 将数据原样返回给客户端 */
    if (send(client_fd, buffer, recv_len, 0) < 0) {
      perror("发送数据失败");
      break;
    }
    printf("[发送] 已回显 %zd 字节\n", recv_len);
  }

  if (recv_len < 0) {
    perror("接收数据失败");
  } else {
    printf("[信息] 客户端 %s 断开连接\n", client_ip);
  }
}

/**
 * 主函数
 */
int main(int argc, char *argv[]) {
  int server_fd, client_fd;       /* 服务器和客户端 Socket */
  struct sockaddr_in server_addr; /* 服务器地址 */
  struct sockaddr_in client_addr; /* 客户端地址 */
  socklen_t client_len;           /* 客户端地址长度 */
  int port;                       /* 监听端口 */
  int opt = 1;                    /* Socket 选项值 */

  /* 解析端口号 */
  if (argc >= 2) {
    port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
      fprintf(stderr, "错误: 无效的端口号 '%s'\n", argv[1]);
      print_usage(argv[0]);
      return EXIT_FAILURE;
    }
  } else {
    port = DEFAULT_PORT;
  }

  printf("========================================\n");
  printf("    TCP ECHO 服务器\n");
  printf("========================================\n");

  /* 步骤1：创建 TCP Socket */
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("创建 Socket 失败");
    return EXIT_FAILURE;
  }
  printf("[信息] Socket 创建成功\n");

  /* 设置 SO_REUSEADDR 选项，避免 "Address already in use" 错误 */
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("设置 Socket 选项失败");
    close(server_fd);
    return EXIT_FAILURE;
  }

  /* 步骤2：配置服务器地址结构 */
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY; /* 监听所有网络接口 */
  server_addr.sin_port = htons(port);

  /* 步骤3：绑定地址 */
  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    perror("绑定地址失败");
    close(server_fd);
    return EXIT_FAILURE;
  }
  printf("[信息] 已绑定到端口 %d\n", port);

  /* 步骤4：开始监听 */
  if (listen(server_fd, BACKLOG) < 0) {
    perror("监听失败");
    close(server_fd);
    return EXIT_FAILURE;
  }
  printf("[信息] 服务器正在监听端口 %d ...\n", port);
  printf("[信息] 按 Ctrl+C 停止服务器\n");
  printf("----------------------------------------\n");

  /* 步骤5：循环接受客户端连接 */
  while (1) {
    client_len = sizeof(client_addr);

    /* 接受新连接 */
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) {
      perror("接受连接失败");
      continue;
    }

    /* 处理客户端请求 */
    handle_client(client_fd, &client_addr);

    /* 关闭客户端连接 */
    close(client_fd);
  }

  /* 关闭服务器 Socket（实际上不会执行到这里） */
  close(server_fd);

  return EXIT_SUCCESS;
}
