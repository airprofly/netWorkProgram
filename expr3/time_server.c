/*
 * time_server.c - UDP TIME 服务器 (RFC 868)
 *
 * 监听 UDP 端口 37，接收任意请求后返回当前时间。
 * 时间格式：自 1900-01-01 00:00:00 以来的秒数，32 位大端序无符号整数。
 */

#define _GNU_SOURCE
#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define TIMEPORT 37
#define TIME_DIFF_1900_TO_1970 2208988800U

int main(int argc, char *argv[]) {
  int port = TIMEPORT;
  if (argc >= 2) {
    port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
      fprintf(stderr, "无效端口号: %s\n", argv[1]);
      return 1;
    }
  }

  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    perror("socket");
    return 1;
  }

  /* 允许端口复用 */
  int opt = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons((uint16_t)port);

  if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    close(sock);
    return 1;
  }

  printf("TIME 服务器已启动，监听端口 %d (UDP)\n", port);
  printf("按 Ctrl+C 停止服务器\n\n");

  while (1) {
    unsigned char buf[64];
    struct sockaddr_in client;
    socklen_t clen = sizeof(client);

    ssize_t n =
        recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *)&client, &clen);
    if (n < 0) {
      perror("recvfrom");
      continue;
    }

    /* 获取当前时间并转换为 1900 纪元 */
    time_t now = time(NULL);
    uint32_t time1900 = (uint32_t)(now + TIME_DIFF_1900_TO_1970);
    uint32_t net_time = htonl(time1900);

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client.sin_addr, client_ip, sizeof(client_ip));
    printf("收到来自 %s:%d 的请求，返回时间\n", client_ip,
           ntohs(client.sin_port));

    /* 发送 4 字节时间 */
    sendto(sock, &net_time, sizeof(net_time), 0, (struct sockaddr *)&client,
           clen);
  }

  close(sock);
  return 0;
}
