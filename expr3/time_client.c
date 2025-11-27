#define _GNU_SOURCE
#include <arpa/inet.h>
#include <netdb.h>
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
  const char *host = (argc >= 2) ? argv[1] : "127.0.0.1";
  int port = (argc >= 3) ? atoi(argv[2]) : TIMEPORT;
  struct addrinfo hints, *res = NULL;
  int sock = -1;
  uint32_t net_time = 0;

  if (port <= 0 || port > 65535) {
    fprintf(stderr, "无效端口号: %s\n", argv[2]);
    return 1;
  }

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; /* IPv4 only (TIME is simple) */
  hints.ai_socktype = SOCK_DGRAM;

  char port_str[6];
  snprintf(port_str, sizeof(port_str), "%d", port);

  if (getaddrinfo(host, port_str, &hints, &res) != 0) {
    perror("getaddrinfo");
    return 1;
  }

  sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (sock < 0) {
    perror("socket");
    freeaddrinfo(res);
    return 1;
  }

  /* Send a single zero byte to request the time (many servers accept empty or
   * any data) */
  unsigned char buf[1] = {0};
  ssize_t sent =
      sendto(sock, buf, sizeof(buf), 0, res->ai_addr, res->ai_addrlen);
  if (sent < 0) {
    perror("sendto");
    close(sock);
    freeaddrinfo(res);
    return 1;
  }

  /* Receive 4 bytes: 32-bit unsigned big-endian seconds since 1900-01-01 */
  struct sockaddr_storage from;
  socklen_t fromlen = sizeof(from);
  ssize_t rec = recvfrom(sock, &net_time, sizeof(net_time), 0,
                         (struct sockaddr *)&from, &fromlen);
  if (rec < 0) {
    perror("recvfrom");
    close(sock);
    freeaddrinfo(res);
    return 1;
  }
  if (rec < 4) {
    fprintf(stderr, "received %zd bytes, expected 4\n", rec);
    close(sock);
    freeaddrinfo(res);
    return 1;
  }

  uint32_t seconds1900 = ntohl(net_time);
  time_t unix_seconds = (time_t)(seconds1900 - TIME_DIFF_1900_TO_1970);

  struct tm tmres;
  if (localtime_r(&unix_seconds, &tmres) == NULL) {
    perror("localtime_r");
    close(sock);
    freeaddrinfo(res);
    return 1;
  }

  char timestr[256];
  if (strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S %z", &tmres) == 0)
    strncpy(timestr, "<time-format-failed>", sizeof(timestr));

  printf("TIME from %s:%d -> %s\n", host, port, timestr);

  close(sock);
  freeaddrinfo(res);
  return 0;
}
