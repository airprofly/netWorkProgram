/**
 * TCP ECHO 客户端程序
 *
 * 功能：连接到 ECHO 服务器，发送用户输入的消息，接收并显示服务器回显的数据
 *
 * 编译：gcc -o echo_client echo_client.c
 * 运行：./echo_client <服务器IP> <端口号>
 * 示例：./echo_client 127.0.0.1 7
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
#define BUFFER_SIZE 1024 /* 缓冲区大小 */
#define DEFAULT_PORT 7   /* ECHO 服务默认端口 */

/**
 * 打印使用说明
 */
void print_usage(const char *program_name) {
  printf("用法: %s <服务器IP> [端口号]\n", program_name);
  printf("示例: %s 127.0.0.1 7\n", program_name);
  printf("说明: 端口号默认为 7 (ECHO 服务标准端口)\n");
}

/**
 * 主函数
 */
int main(int argc, char *argv[]) {
  int sock_fd;                    /* Socket 文件描述符 */
  struct sockaddr_in server_addr; /* 服务器地址结构 */
  char send_buffer[BUFFER_SIZE];  /* 发送缓冲区 */
  char recv_buffer[BUFFER_SIZE];  /* 接收缓冲区 */
  int port;                       /* 服务器端口号 */
  ssize_t send_len, recv_len;     /* 发送和接收的字节数 */

  /* 步骤1：参数检查 */
  if (argc < 2) {
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  /* 解析端口号，如果未指定则使用默认端口 */
  if (argc >= 3) {
    port = atoi(argv[2]);
    if (port <= 0 || port > 65535) {
      fprintf(stderr, "错误: 无效的端口号 '%s'，端口范围应为 1-65535\n",
              argv[2]);
      return EXIT_FAILURE;
    }
  } else {
    port = DEFAULT_PORT;
  }

  printf("========================================\n");
  printf("    TCP ECHO 客户端\n");
  printf("========================================\n");
  printf("目标服务器: %s:%d\n\n", argv[1], port);

  /* 步骤2：创建 TCP Socket */
  sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd < 0) {
    perror("创建 Socket 失败");
    return EXIT_FAILURE;
  }
  printf("[信息] Socket 创建成功\n");

  /* 步骤3：配置服务器地址结构 */
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);

  /* 转换 IP 地址 */
  if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
    fprintf(stderr, "错误: 无效的 IP 地址 '%s'\n", argv[1]);
    close(sock_fd);
    return EXIT_FAILURE;
  }

  /* 步骤4：连接服务器 */
  printf("[信息] 正在连接服务器 %s:%d ...\n", argv[1], port);
  if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    perror("连接服务器失败");
    close(sock_fd);
    return EXIT_FAILURE;
  }
  printf("[信息] 连接成功！\n\n");

  /* 步骤5：数据交互循环 */
  printf("提示: 输入要发送的消息，输入 'quit' 或 'exit' 退出程序\n");
  printf("----------------------------------------\n");

  while (1) {
    /* 显示提示符并读取用户输入 */
    printf("\n发送> ");
    fflush(stdout);

    /* 读取一行输入 */
    if (fgets(send_buffer, BUFFER_SIZE, stdin) == NULL) {
      printf("\n[信息] 检测到输入结束，退出程序\n");
      break;
    }

    /* 移除换行符 */
    size_t len = strlen(send_buffer);
    if (len > 0 && send_buffer[len - 1] == '\n') {
      send_buffer[len - 1] = '\0';
      len--;
    }

    /* 检查是否为空输入 */
    if (len == 0) {
      printf("[提示] 输入为空，请重新输入\n");
      continue;
    }

    /* 检查退出命令 */
    if (strcmp(send_buffer, "quit") == 0 || strcmp(send_buffer, "exit") == 0) {
      printf("[信息] 用户请求退出\n");
      break;
    }

    /* 发送数据到服务器 */
    send_len = send(sock_fd, send_buffer, len, 0);
    if (send_len < 0) {
      perror("发送数据失败");
      break;
    }
    printf("[发送] 已发送 %zd 字节\n", send_len);

    /* 接收服务器回显 */
    memset(recv_buffer, 0, BUFFER_SIZE);
    recv_len = recv(sock_fd, recv_buffer, BUFFER_SIZE - 1, 0);

    if (recv_len < 0) {
      perror("接收数据失败");
      break;
    } else if (recv_len == 0) {
      printf("[信息] 服务器关闭了连接\n");
      break;
    }

    /* 显示接收到的回显数据 */
    recv_buffer[recv_len] = '\0';
    printf("[接收] 收到 %zd 字节: %s\n", recv_len, recv_buffer);

    /* 验证回显是否正确 */
    if (strcmp(send_buffer, recv_buffer) == 0) {
      printf("[验证] ✓ 回显数据与发送数据一致\n");
    } else {
      printf("[验证] ✗ 回显数据与发送数据不一致\n");
    }
  }

  /* 步骤6：关闭连接，清理资源 */
  printf("\n----------------------------------------\n");
  printf("[信息] 正在关闭连接...\n");
  close(sock_fd);
  printf("[信息] 程序结束\n");

  return EXIT_SUCCESS;
}
