/**
 * TCP聊天服务器
 * 功能：接受多个客户端连接，转发消息给其他客户端
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define PORT 8888

// 客户端信息结构体
typedef struct {
  int sockfd;
  struct sockaddr_in addr;
  char name[32];
  int active;
} ClientInfo;

// 全局变量
ClientInfo clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
int server_running = 1;

// 函数声明
void *handle_client(void *arg);
void broadcast_message(const char *message, int sender_idx);
void send_private_message(const char *message, const char *target_name,
                          int sender_idx);
void remove_client(int idx);
int get_client_count();
void signal_handler(int sig);

int main() {
  int server_fd, client_fd;
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len = sizeof(client_addr);
  pthread_t tid;

  // 设置信号处理
  signal(SIGINT, signal_handler);
  signal(SIGPIPE, SIG_IGN); // 忽略SIGPIPE信号

  // 初始化客户端数组
  for (int i = 0; i < MAX_CLIENTS; i++) {
    clients[i].sockfd = -1;
    clients[i].active = 0;
  }

  // 创建socket
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("创建socket失败");
    exit(EXIT_FAILURE);
  }

  // 设置socket选项，允许地址重用
  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("设置socket选项失败");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  // 配置服务器地址
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  // 绑定
  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    perror("绑定失败");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  // 监听
  if (listen(server_fd, MAX_CLIENTS) < 0) {
    perror("监听失败");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  printf("========================================\n");
  printf("   TCP聊天服务器已启动\n");
  printf("   监听端口: %d\n", PORT);
  printf("   最大连接数: %d\n", MAX_CLIENTS);
  printf("   按 Ctrl+C 关闭服务器\n");
  printf("========================================\n\n");

  // 主循环：接受客户端连接
  while (server_running) {
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) {
      if (server_running) {
        perror("接受连接失败");
      }
      continue;
    }

    // 检查是否达到最大连接数
    pthread_mutex_lock(&clients_mutex);
    int idx = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (clients[i].active == 0) {
        idx = i;
        break;
      }
    }

    if (idx == -1) {
      pthread_mutex_unlock(&clients_mutex);
      printf("连接已满，拒绝新连接: %s:%d\n", inet_ntoa(client_addr.sin_addr),
             ntohs(client_addr.sin_port));
      const char *msg = "服务器已满，请稍后再试。\n";
      send(client_fd, msg, strlen(msg), 0);
      close(client_fd);
      continue;
    }

    // 添加新客户端
    clients[idx].sockfd = client_fd;
    clients[idx].addr = client_addr;
    clients[idx].active = 1;
    snprintf(clients[idx].name, sizeof(clients[idx].name), "用户%d", idx + 1);
    pthread_mutex_unlock(&clients_mutex);

    printf("[+] 新客户端连接: %s:%d (分配为 %s)\n",
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port),
           clients[idx].name);
    printf("    当前在线人数: %d\n", get_client_count());

    // 创建线程处理客户端
    int *client_idx = malloc(sizeof(int));
    *client_idx = idx;
    if (pthread_create(&tid, NULL, handle_client, client_idx) != 0) {
      perror("创建线程失败");
      remove_client(idx);
      free(client_idx);
      continue;
    }
    pthread_detach(tid);
  }

  // 清理
  close(server_fd);
  printf("\n服务器已关闭\n");
  return 0;
}

// 处理客户端消息的线程函数
void *handle_client(void *arg) {
  int idx = *((int *)arg);
  free(arg);

  char buffer[BUFFER_SIZE];
  char message[BUFFER_SIZE + 64];
  int bytes_received;

  // 发送欢迎消息
  snprintf(buffer, sizeof(buffer),
           "欢迎来到聊天室！你的昵称是: %s\n"
           "命令列表:\n"
           "  /quit          - 退出聊天室\n"
           "  /name <昵称>   - 修改昵称\n"
           "  /list          - 查看在线用户\n"
           "  /msg <用户> <消息> - 私聊指定用户\n"
           "直接输入消息则广播给所有人\n",
           clients[idx].name);
  send(clients[idx].sockfd, buffer, strlen(buffer), 0);

  // 通知其他用户
  snprintf(message, sizeof(message), "[系统] %s 加入了聊天室\n",
           clients[idx].name);
  broadcast_message(message, idx);

  // 接收并转发消息
  while (server_running && clients[idx].active) {
    memset(buffer, 0, sizeof(buffer));
    bytes_received = recv(clients[idx].sockfd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received <= 0) {
      // 客户端断开连接
      break;
    }

    // 移除末尾的换行符
    buffer[strcspn(buffer, "\r\n")] = '\0';

    if (strlen(buffer) == 0) {
      continue;
    }

    // 处理命令
    if (strncmp(buffer, "/quit", 5) == 0) {
      break;
    } else if (strncmp(buffer, "/name ", 6) == 0) {
      // 修改昵称
      char old_name[32];
      pthread_mutex_lock(&clients_mutex);
      strncpy(old_name, clients[idx].name, sizeof(old_name) - 1);
      strncpy(clients[idx].name, buffer + 6, sizeof(clients[idx].name) - 1);
      clients[idx].name[sizeof(clients[idx].name) - 1] = '\0';
      pthread_mutex_unlock(&clients_mutex);

      snprintf(message, sizeof(message), "[系统] %s 改名为 %s\n", old_name,
               clients[idx].name);
      broadcast_message(message, -1);
      printf("[*] %s 改名为 %s\n", old_name, clients[idx].name);
    } else if (strncmp(buffer, "/list", 5) == 0) {
      // 显示在线用户
      char list_msg[BUFFER_SIZE] = "[在线用户列表]\n";
      pthread_mutex_lock(&clients_mutex);
      for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
          char user_info[64];
          snprintf(user_info, sizeof(user_info), "  - %s%s\n", clients[i].name,
                   (i == idx) ? " (你)" : "");
          strncat(list_msg, user_info, sizeof(list_msg) - strlen(list_msg) - 1);
        }
      }
      pthread_mutex_unlock(&clients_mutex);
      send(clients[idx].sockfd, list_msg, strlen(list_msg), 0);
    } else if (strncmp(buffer, "/msg ", 5) == 0) {
      // 私聊功能: /msg <用户名> <消息>
      char *cmd_content = buffer + 5;
      char target_name[32] = {0};
      char *space_pos = strchr(cmd_content, ' ');

      if (space_pos == NULL) {
        const char *usage = "[系统] 用法: /msg <用户名> <消息>\n";
        send(clients[idx].sockfd, usage, strlen(usage), 0);
      } else {
        // 提取目标用户名
        size_t name_len = space_pos - cmd_content;
        if (name_len >= sizeof(target_name)) {
          name_len = sizeof(target_name) - 1;
        }
        strncpy(target_name, cmd_content, name_len);
        target_name[name_len] = '\0';

        // 提取消息内容
        char *private_msg = space_pos + 1;

        if (strlen(private_msg) == 0) {
          const char *empty_msg = "[系统] 消息内容不能为空\n";
          send(clients[idx].sockfd, empty_msg, strlen(empty_msg), 0);
        } else {
          // 构建私聊消息
          snprintf(message, sizeof(message), "[私聊][%s -> 你]: %s\n",
                   clients[idx].name, private_msg);
          send_private_message(message, target_name, idx);

          // 给发送者确认
          snprintf(message, sizeof(message), "[私聊][你 -> %s]: %s\n",
                   target_name, private_msg);
          send(clients[idx].sockfd, message, strlen(message), 0);

          printf("[私聊] %s -> %s: %s\n", clients[idx].name, target_name,
                 private_msg);
        }
      }
    } else if (strncmp(buffer, "/msg", 4) == 0) {
      // 用户输入了 /msg 但格式不对（如 /msg用户2）
      const char *usage =
          "[系统] 私聊格式错误！正确用法: /msg 用户名 消息内容\n"
          "[系统] 注意: /msg 后面必须有空格\n"
          "[系统] 示例: /msg 用户2 你好\n";
      send(clients[idx].sockfd, usage, strlen(usage), 0);
    } else {
      // 广播普通消息
      snprintf(message, sizeof(message), "[%s]: %s\n", clients[idx].name,
               buffer);
      broadcast_message(message, idx);
      printf("[消息] %s: %s\n", clients[idx].name, buffer);
    }
  }

  // 客户端断开连接
  char name_copy[32];
  strncpy(name_copy, clients[idx].name, sizeof(name_copy) - 1);
  name_copy[sizeof(name_copy) - 1] = '\0';

  remove_client(idx);

  printf("[-] %s 已断开连接\n", name_copy);
  printf("    当前在线人数: %d\n", get_client_count());

  snprintf(message, sizeof(message), "[系统] %s 离开了聊天室\n", name_copy);
  broadcast_message(message, -1);

  return NULL;
}

// 广播消息给所有客户端（除了发送者）
void broadcast_message(const char *message, int sender_idx) {
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].active && i != sender_idx) {
      if (send(clients[i].sockfd, message, strlen(message), 0) < 0) {
        // 发送失败，标记为不活跃
        clients[i].active = 0;
      }
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}

// 发送私聊消息给指定用户
void send_private_message(const char *message, const char *target_name,
                          int sender_idx) {
  int found = 0;
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].active && strcmp(clients[i].name, target_name) == 0) {
      if (send(clients[i].sockfd, message, strlen(message), 0) < 0) {
        clients[i].active = 0;
      }
      found = 1;
      break;
    }
  }
  pthread_mutex_unlock(&clients_mutex);

  // 如果目标用户不存在，通知发送者
  if (!found) {
    char error_msg[128];
    snprintf(error_msg, sizeof(error_msg), "[系统] 用户 '%s' 不在线或不存在\n",
             target_name);
    send(clients[sender_idx].sockfd, error_msg, strlen(error_msg), 0);
  }
}

// 移除客户端
void remove_client(int idx) {
  pthread_mutex_lock(&clients_mutex);
  if (clients[idx].active) {
    close(clients[idx].sockfd);
    clients[idx].sockfd = -1;
    clients[idx].active = 0;
    memset(clients[idx].name, 0, sizeof(clients[idx].name));
  }
  pthread_mutex_unlock(&clients_mutex);
}

// 获取当前在线客户端数量
int get_client_count() {
  int count = 0;
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].active) {
      count++;
    }
  }
  pthread_mutex_unlock(&clients_mutex);
  return count;
}

// 信号处理函数
void signal_handler(int sig) {
  (void)sig; // 抑制未使用参数警告
  printf("\n接收到关闭信号，正在关闭服务器...\n");
  server_running = 0;

  // 关闭所有客户端连接
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].active) {
      const char *msg = "[系统] 服务器关闭\n";
      send(clients[i].sockfd, msg, strlen(msg), 0);
      close(clients[i].sockfd);
      clients[i].active = 0;
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}
