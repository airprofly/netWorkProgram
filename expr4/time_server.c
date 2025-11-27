/**
 * time_server.c - 无连接TIME服务器
 *
 * 使用UDP协议实现RFC 868 TIME协议服务器
 *
 * 功能：
 * 1. 监听指定UDP端口
 * 2. 接收客户端请求
 * 3. 返回当前时间（从1900年1月1日开始的秒数）
 *
 * 用法：./time_server [port]
 */

#include "common.h"

int main(int argc, char *argv[])
{
    int sockfd;                     /* UDP套接字描述符 */
    struct sockaddr_in server_addr; /* 服务器地址结构 */
    struct sockaddr_in client_addr; /* 客户端地址结构 */
    socklen_t client_len;           /* 客户端地址长度 */
    int port;                       /* 服务端口号 */
    char buffer[BUFFER_SIZE];       /* 接收缓冲区 */
    ssize_t recv_len;               /* 接收数据长度 */
    time_t current_time;            /* 当前Unix时间 */
    uint32_t time_value;            /* TIME协议时间值 */
    uint32_t network_time;          /* 网络字节序时间值 */
    char time_str[64];              /* 时间字符串 */

    /* 解析端口参数 */
    if (argc > 1)
    {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535)
        {
            fprintf(stderr, "Invalid port number: %s\n", argv[1]);
            fprintf(stderr, "Usage: %s [port]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        port = TIME_PORT;
    }

    /* 创建UDP套接字 */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        error_exit("Failed to create socket");
    }

    /* 设置套接字选项，允许地址重用 */
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        error_exit("Failed to set socket option");
    }

    /* 初始化服务器地址结构 */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;         /* IPv4 */
    server_addr.sin_addr.s_addr = INADDR_ANY; /* 监听所有网络接口 */
    server_addr.sin_port = htons(port);       /* 端口号（网络字节序） */

    /* 绑定套接字到指定端口 */
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        close(sockfd);
        error_exit("Failed to bind socket");
    }

    printf("===========================================\n");
    printf("     无连接TIME服务器 (UDP)\n");
    printf("===========================================\n");
    printf("TIME Server started on port %d\n", port);
    printf("Waiting for client requests...\n");
    printf("Press Ctrl+C to stop the server\n");
    printf("===========================================\n\n");

    /* 主循环：等待并处理客户端请求 */
    while (1)
    {
        client_len = sizeof(client_addr);

        /* 接收客户端请求（阻塞等待） */
        /* 对于TIME协议，客户端只需发送任意数据即可请求时间 */
        recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                            (struct sockaddr *)&client_addr, &client_len);

        if (recv_len < 0)
        {
            perror("Failed to receive data");
            continue;
        }

        /* 获取当前时间 */
        current_time = time(NULL);

        /* 转换为TIME协议时间（从1900年开始的秒数） */
        time_value = unix_to_time_protocol(current_time);

        /* 转换为网络字节序（大端序） */
        network_time = htonl(time_value);

        /* 获取可读的时间字符串 */
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S",
                 localtime(&current_time));

        /* 打印客户端信息 */
        printf("[Request] From %s:%d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));
        printf("  TIME value: %u\n", time_value);
        printf("  Local time: %s\n\n", time_str);

        /* 发送时间值给客户端 */
        if (sendto(sockfd, &network_time, sizeof(network_time), 0,
                   (struct sockaddr *)&client_addr, client_len) < 0)
        {
            perror("Failed to send data");
            continue;
        }
    }

    /* 关闭套接字（实际上不会执行到这里） */
    close(sockfd);
    return 0;
}
