#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
  if (argc <= 2) {
    printf("usage:%s ip_address port_number\n", basename(argv[0]));
    return 1;
  }
  const char *ip = argv[1];
  int port = atoi(argv[2]);
  struct sockaddr_in server_address;
  bzero(&server_address, sizeof(server_address));
  server_address.sin_family = AF_INET;
  inet_pton(AF_INET, ip, &server_address.sin_addr);
  server_address.sin_port = htons(port);
  int sockfd = socket(PF_INET, SOCK_STREAM, 0);
  assert(sockfd >= 0);

  if (connect(sockfd, (struct sockaddr *)&server_address,
              sizeof(server_address)) < 0) {
    printf("connection failed\n");
  } else {

    // const char *normal_data = "123";
    // send(sockfd, normal_data, strlen(normal_data), 0);

    char buffer[BUF_SIZE];
    memset(buffer, '\0', BUF_SIZE);
    int ret = 0;
    ret = recv(sockfd, buffer, BUF_SIZE - 1, 0);
    printf("got %d bytes of normal data '\n%s\n'\n", ret, buffer);

    memset(buffer, '\0', BUF_SIZE);
    ret = recv(sockfd, buffer, BUF_SIZE - 1, 0);
    if (ret == -1) {
      printf("Error: %s\n", strerror(errno));
    } else {
      printf("got %d bytes of oob data '%s'\n", ret, buffer);
    }

    memset(buffer, '\0', BUF_SIZE);
    ret = recv(sockfd, buffer, BUF_SIZE - 1, 0);
    printf("got %d bytes of normal data '%s'\n", ret, buffer);
  }
  close(sockfd);
  return 0;
}