/**
 * common.h - 公共头文件
 *
 * 定义TIME服务器和客户端共用的常量和结构
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

/* TIME协议默认端口 */
#define TIME_PORT 37

/* 从1900年1月1日到1970年1月1日的秒数差 */
/* TIME协议使用1900年为基准，而Unix时间戳使用1970年为基准 */
#define TIME_OFFSET 2208988800UL

/* 缓冲区大小 */
#define BUFFER_SIZE 1024

/* 超时时间（秒） */
#define TIMEOUT_SEC 5

/**
 * 错误处理函数
 * @param msg 错误信息
 */
static inline void error_exit(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

/**
 * 将Unix时间戳转换为TIME协议时间
 * @param unix_time Unix时间戳
 * @return TIME协议时间（从1900年开始的秒数）
 */
static inline uint32_t unix_to_time_protocol(time_t unix_time)
{
    return (uint32_t)(unix_time + TIME_OFFSET);
}

/**
 * 将TIME协议时间转换为Unix时间戳
 * @param time_protocol TIME协议时间
 * @return Unix时间戳
 */
static inline time_t time_protocol_to_unix(uint32_t time_protocol)
{
    return (time_t)(time_protocol - TIME_OFFSET);
}

#endif /* COMMON_H */
