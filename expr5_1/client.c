/**
 * TCP聊天客户端
 * 功能：连接服务器，发送和接收聊天消息
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define BUFFER_SIZE 1024
#define DEFAULT_PORT 8888
#define DEFAULT_SERVER "127.0.0.1"

// 全局变量
int client_running = 1;
int sockfd = -1;

// 函数声明
void *receive_messages(void *arg);
void signal_handler(int sig);
void print_usage(const char *program);

int main(int argc, char *argv[])
{
    struct sockaddr_in server_addr;
    pthread_t recv_thread;
    char buffer[BUFFER_SIZE];
    char *server_ip = DEFAULT_SERVER;
    int port = DEFAULT_PORT;

    // 解析命令行参数
    if (argc >= 2)
    {
        server_ip = argv[1];
    }
    if (argc >= 3)
    {
        port = atoi(argv[2]);
        if (port <= 0 || port > 65535)
        {
            fprintf(stderr, "无效的端口号: %s\n", argv[2]);
            exit(EXIT_FAILURE);
        }
    }

    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGPIPE, SIG_IGN);

    // 创建socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("创建socket失败");
        exit(EXIT_FAILURE);
    }

    // 配置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
    {
        fprintf(stderr, "无效的服务器地址: %s\n", server_ip);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("========================================\n");
    printf("   TCP聊天客户端\n");
    printf("   正在连接 %s:%d ...\n", server_ip, port);
    printf("========================================\n");

    // 连接服务器
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("连接服务器失败");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("连接成功！\n\n");

    // 创建接收消息的线程
    if (pthread_create(&recv_thread, NULL, receive_messages, NULL) != 0)
    {
        perror("创建接收线程失败");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // 主循环：发送消息
    while (client_running)
    {
        memset(buffer, 0, sizeof(buffer));

        // 读取用户输入
        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        {
            break;
        }

        // 检查是否退出
        if (strncmp(buffer, "/quit", 5) == 0)
        {
            printf("正在退出...\n");
            send(sockfd, buffer, strlen(buffer), 0);
            break;
        }

        // 发送消息
        if (strlen(buffer) > 0)
        {
            if (send(sockfd, buffer, strlen(buffer), 0) < 0)
            {
                if (client_running)
                {
                    perror("发送消息失败");
                }
                break;
            }
        }
    }

    // 清理
    client_running = 0;
    close(sockfd);
    sockfd = -1;

    // 等待接收线程结束
    pthread_cancel(recv_thread);
    pthread_join(recv_thread, NULL);

    printf("\n已断开连接\n");
    return 0;
}

// 接收消息的线程函数
void *receive_messages(void *arg)
{
    (void)arg; // 抑制未使用参数警告
    char buffer[BUFFER_SIZE];
    int bytes_received;

    while (client_running)
    {
        memset(buffer, 0, sizeof(buffer));
        bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received <= 0)
        {
            if (client_running)
            {
                printf("\n与服务器的连接已断开\n");
                client_running = 0;
            }
            break;
        }

        // 显示收到的消息
        printf("%s", buffer);
        fflush(stdout);
    }

    return NULL;
}

// 信号处理函数
void signal_handler(int sig)
{
    (void)sig; // 抑制未使用参数警告
    printf("\n接收到退出信号\n");
    client_running = 0;
    if (sockfd >= 0)
    {
        const char *quit_msg = "/quit\n";
        send(sockfd, quit_msg, strlen(quit_msg), 0);
        close(sockfd);
        sockfd = -1;
    }
    exit(0);
}

// 打印使用说明
void print_usage(const char *program)
{
    printf("使用方法: %s [服务器IP] [端口]\n", program);
    printf("  服务器IP: 服务器的IP地址 (默认: %s)\n", DEFAULT_SERVER);
    printf("  端口:     服务器的端口号 (默认: %d)\n", DEFAULT_PORT);
    printf("\n示例:\n");
    printf("  %s                    # 连接本地服务器\n", program);
    printf("  %s 192.168.1.100      # 连接指定IP\n", program);
    printf("  %s 192.168.1.100 9999 # 连接指定IP和端口\n", program);
}
