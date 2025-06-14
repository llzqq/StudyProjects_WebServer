#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/**
 * @brief 获取目标主机地址信息，以及获取目标服务信息
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char *argv[]) {
  assert(argc == 2);
  char *host = argv[1];
  /*获取目标主机地址信息*/
  struct hostent *hostinfo = gethostbyname(host);
  assert(hostinfo);
  /*获取daytime服务信息*/
  struct servent *servinfo = getservbyname("daytime", "tcp");
  assert(servinfo);
  printf("daytime port is %d\n", ntohs(servinfo->s_port));
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_port = servinfo->s_port;
  /*注意下面的代码，因为h_addr_list本身是使用网络字节序的地址列表，所以使用其
  中的IP地址时，无须对目标IP地址转换字节序*/
  address.sin_addr = *(struct in_addr *)*hostinfo->h_addr_list;
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  int result = connect(sockfd, (struct sockaddr *)&address, sizeof(address));
  // assert(result != -1);
  if (result == -1) {
    printf("Error: %s\n", strerror(errno));
    close(sockfd);
    return 1;
  }
  char buffer[128];
  result = read(sockfd, buffer, sizeof(buffer));
  assert(result > 0);
  buffer[result] = '\0';
  printf("the day tiem is:%s", buffer);
  close(sockfd);
  return 0;
}