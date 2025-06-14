#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static bool stop = false;

/*SIGTERM信号的处理函数，触发时结束主程序中的循环*/
static void handle_term(int sig) { stop = true; }

int main(int argc, char *argv[]) {
  signal(SIGTERM, handle_term);
  if (argc <= 3) {
    printf("usage:%s ip_address port_number backlog\n", basename(argv[0]));
    return 1;
  }
  const char *ip = argv[1];
  int port = atoi(argv[2]);
  int backlog = atoi(argv[3]);

  // 创建socket
  int sock = socket(PF_INET, SOCK_STREAM, 0);
  assert(sock >= 0);

  /*创建一个IPv4 socket地址*/
  struct sockaddr_in address;
  bzero(&address, sizeof(address));
  address.sin_family = AF_INET;
  inet_pton(AF_INET, ip, &address.sin_addr);
  address.sin_port = htons(port);
  // 命名 socket，并且强制转换地址类型
  int ret = bind(sock, (struct sockaddr *)&address, sizeof(address));
  assert(ret != -1);

  // 监听 socket
  ret = listen(sock, backlog);
  assert(ret != -1);

  /*循环等待连接，直到有SIGTERM信号将它中断*/
  while (!stop) {
    sleep(1);
  }

  /*关闭socket，见后文*/
  close(sock);
  return 0;
}