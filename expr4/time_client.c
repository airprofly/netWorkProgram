/**
 * time_client.c - 无连接TIME客户端
 *
 * 使用UDP协议实现RFC 868 TIME协议客户端
 *
 * 功能：
 * 1. 向TIME服务器发送请求
 * 2. 接收服务器返回的时间值
 * 3. 将时间值转换为可读格式并显示
 *
 * 用法：./time_client <server_ip> [port]
 */

#include "common.h"

int main(int argc, char *argv[])
{
    int sockfd;                     /* UDP套接字描述符 */
    struct sockaddr_in server_addr; /* 服务器地址结构 */
    socklen_t server_len;           /* 服务器地址长度 */
    int port;                       /* 服务端口号 */
    const char *server_ip;          /* 服务器IP地址 */
    char request[] = "TIME";        /* 请求消息（内容不重要） */
    uint32_t network_time;          /* 网络字节序时间值 */
    uint32_t time_value;            /* TIME协议时间值 */
    time_t unix_time;               /* Unix时间戳 */
    char time_str[64];              /* 时间字符串 */
    struct timeval timeout;         /* 超时设置 */
    ssize_t recv_len;               /* 接收数据长度 */

    /* 检查参数 */
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <server_ip> [port]\n", argv[0]);
        fprintf(stderr, "Example: %s 127.0.0.1 8037\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    server_ip = argv[1];

    /* 解析端口参数 */
    if (argc > 2)
    {
        port = atoi(argv[2]);
        if (port <= 0 || port > 65535)
        {
            fprintf(stderr, "Invalid port number: %s\n", argv[2]);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        port = TIME_PORT;
    }

    printf("===========================================\n");
    printf("     无连接TIME客户端 (UDP)\n");
    printf("===========================================\n");
    printf("Connecting to TIME server %s:%d\n", server_ip, port);
    printf("===========================================\n\n");

    /* 创建UDP套接字 */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        error_exit("Failed to create socket");
    }

    /* 设置接收超时 */
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        perror("Warning: Failed to set timeout");
    }

    /* 初始化服务器地址结构 */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;   /* IPv4 */
    server_addr.sin_port = htons(port); /* 端口号（网络字节序） */

    /* 转换IP地址 */
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
    {
        close(sockfd);
        fprintf(stderr, "Invalid server IP address: %s\n", server_ip);
        exit(EXIT_FAILURE);
    }

    /* 发送时间请求 */
    printf("Sending time request...\n");
    if (sendto(sockfd, request, strlen(request), 0,
               (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        close(sockfd);
        error_exit("Failed to send request");
    }

    /* 接收服务器响应 */
    server_len = sizeof(server_addr);
    recv_len = recvfrom(sockfd, &network_time, sizeof(network_time), 0,
                        (struct sockaddr *)&server_addr, &server_len);

    if (recv_len < 0)
    {
        close(sockfd);
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            fprintf(stderr, "Error: Request timed out (no response from server)\n");
        }
        else
        {
            perror("Failed to receive response");
        }
        exit(EXIT_FAILURE);
    }

    if (recv_len != sizeof(network_time))
    {
        close(sockfd);
        fprintf(stderr, "Error: Invalid response size (expected %zu, got %zd)\n",
                sizeof(network_time), recv_len);
        exit(EXIT_FAILURE);
    }

    /* 转换字节序 */
    time_value = ntohl(network_time);

    /* 转换为Unix时间戳 */
    unix_time = time_protocol_to_unix(time_value);

    /* 获取可读的时间字符串 */
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S",
             localtime(&unix_time));

    /* 显示结果 */
    printf("\n===========================================\n");
    printf("           Response Received\n");
    printf("===========================================\n");
    printf("TIME protocol value : %u\n", time_value);
    printf("Unix timestamp      : %ld\n", (long)unix_time);
    printf("Local time          : %s\n", time_str);
    printf("===========================================\n");

    /* 与本地时间比较 */
    time_t local_time = time(NULL);
    long diff = (long)(unix_time - local_time);
    printf("\nTime difference from local: %ld seconds\n", diff);

    /* 关闭套接字 */
    close(sockfd);

    return 0;
}
