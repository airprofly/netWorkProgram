/**
 * ECHO客户端程序
 *
 * 功能：连接到ECHO服务器，发送用户输入的数据，并接收服务器回显的数据
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define DEFAULT_PORT 8888      // 默认服务器端口
#define DEFAULT_IP "127.0.0.1" // 默认服务器IP
#define BUFFER_SIZE 1024       // 缓冲区大小

/**
 * 错误处理函数
 */
void error_exit(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  int sock_fd;
  struct sockaddr_in server_addr;
  char send_buffer[BUFFER_SIZE];
  char recv_buffer[BUFFER_SIZE];
  ssize_t bytes_sent, bytes_received;

  const char *server_ip = DEFAULT_IP;
  int port = DEFAULT_PORT;

  // 解析命令行参数
  if (argc > 1) {
    server_ip = argv[1];
  }
  if (argc > 2) {
    port = atoi(argv[2]);
    if (port <= 0 || port > 65535) {
      fprintf(stderr, "无效的端口号: %s\n", argv[2]);
      exit(EXIT_FAILURE);
    }
  }

  // 1. 创建TCP套接字
  sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd < 0) {
    error_exit("创建套接字失败");
  }
  printf("套接字创建成功\n");

  // 2. 设置服务器地址
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);

  if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
    fprintf(stderr, "无效的IP地址: %s\n", server_ip);
    close(sock_fd);
    exit(EXIT_FAILURE);
  }

  // 3. 连接到服务器
  printf("正在连接到服务器 %s:%d...\n", server_ip, port);
  if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    error_exit("连接服务器失败");
  }
  printf("连接成功！\n");
  printf("输入要发送的内容（输入 'quit' 退出）：\n\n");

  // 4. 循环发送和接收数据
  while (1) {
    printf("> ");
    fflush(stdout);

    // 读取用户输入
    if (fgets(send_buffer, BUFFER_SIZE, stdin) == NULL) {
      printf("\n检测到EOF，退出...\n");
      break;
    }

    // 检查是否退出
    if (strncmp(send_buffer, "quit", 4) == 0) {
      printf("正在断开连接...\n");
      break;
    }

    // 发送数据到服务器
    size_t msg_len = strlen(send_buffer);
    bytes_sent = send(sock_fd, send_buffer, msg_len, 0);
    if (bytes_sent < 0) {
      perror("发送数据失败");
      break;
    }

    // 接收服务器回显的数据
    bytes_received = recv(sock_fd, recv_buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received < 0) {
      perror("接收数据失败");
      break;
    } else if (bytes_received == 0) {
      printf("服务器断开连接\n");
      break;
    }

    recv_buffer[bytes_received] = '\0';
    printf("服务器回显: %s", recv_buffer);
  }

  // 5. 关闭套接字
  close(sock_fd);
  printf("连接已关闭\n");

  return 0;
}
