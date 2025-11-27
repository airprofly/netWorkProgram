/**
 * 并发的面向连接的ECHO服务器
 *
 * 功能：接收客户端发送的数据，并将其原样返回（回显）
 * 实现方式：使用fork()创建子进程处理每个客户端连接，实现并发服务
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PORT 8888        // 服务器监听端口
#define BUFFER_SIZE 1024 // 缓冲区大小
#define BACKLOG 10       // 最大等待连接队列长度

/**
 * 信号处理函数：处理子进程终止信号，避免僵尸进程
 */
void sigchld_handler(int signo) {
  (void)signo; // 避免未使用参数警告
  // 回收所有已终止的子进程
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;
}

/**
 * 错误处理函数
 */
void error_exit(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

/**
 * 处理客户端连接的函数
 * 实现ECHO功能：接收数据并原样返回
 */
void handle_client(int client_fd, struct sockaddr_in *client_addr) {
  char buffer[BUFFER_SIZE];
  ssize_t bytes_received;
  char client_ip[INET_ADDRSTRLEN];

  // 获取客户端IP地址
  inet_ntop(AF_INET, &(client_addr->sin_addr), client_ip, INET_ADDRSTRLEN);
  printf("[子进程 %d] 开始处理客户端 %s:%d\n", getpid(), client_ip,
         ntohs(client_addr->sin_port));

  // 循环接收并回显数据
  while ((bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
    buffer[bytes_received] = '\0';
    printf("[子进程 %d] 收到数据: %s", getpid(), buffer);

    // 将数据原样发送回客户端（ECHO）
    if (send(client_fd, buffer, bytes_received, 0) < 0) {
      perror("发送数据失败");
      break;
    }
    printf("[子进程 %d] 已回显数据\n", getpid());
  }

  if (bytes_received < 0) {
    perror("接收数据失败");
  } else {
    printf("[子进程 %d] 客户端 %s:%d 已断开连接\n", getpid(), client_ip,
           ntohs(client_addr->sin_port));
  }

  close(client_fd);
}

int main(int argc, char *argv[]) {
  int server_fd, client_fd;
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len;
  pid_t pid;
  int port = PORT;

  // 可通过命令行参数指定端口
  if (argc > 1) {
    port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
      fprintf(stderr, "无效的端口号: %s\n", argv[1]);
      exit(EXIT_FAILURE);
    }
  }

  // 设置SIGCHLD信号处理，避免僵尸进程
  struct sigaction sa;
  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
  if (sigaction(SIGCHLD, &sa, NULL) < 0) {
    error_exit("设置信号处理失败");
  }

  // 1. 创建TCP套接字
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    error_exit("创建套接字失败");
  }
  printf("套接字创建成功\n");

  // 设置套接字选项，允许地址重用
  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    error_exit("设置套接字选项失败");
  }

  // 2. 绑定地址和端口
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY; // 监听所有网络接口
  server_addr.sin_port = htons(port);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    error_exit("绑定地址失败");
  }
  printf("绑定端口 %d 成功\n", port);

  // 3. 开始监听
  if (listen(server_fd, BACKLOG) < 0) {
    error_exit("监听失败");
  }
  printf("服务器正在监听端口 %d...\n", port);
  printf("等待客户端连接...\n\n");

  // 4. 主循环：接受连接并创建子进程处理
  while (1) {
    client_len = sizeof(client_addr);

    // 接受客户端连接
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) {
      if (errno == EINTR) {
        // 被信号中断，继续等待
        continue;
      }
      perror("接受连接失败");
      continue;
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    printf("[主进程] 接受来自 %s:%d 的连接\n", client_ip,
           ntohs(client_addr.sin_port));

    // 创建子进程处理客户端请求
    pid = fork();
    if (pid < 0) {
      perror("创建子进程失败");
      close(client_fd);
      continue;
    } else if (pid == 0) {
      // 子进程
      close(server_fd); // 子进程不需要监听套接字
      handle_client(client_fd, &client_addr);
      exit(EXIT_SUCCESS);
    } else {
      // 父进程
      printf("[主进程] 创建子进程 %d 处理客户端\n\n", pid);
      close(client_fd); // 父进程不需要客户端套接字
    }
  }

  close(server_fd);
  return 0;
}
