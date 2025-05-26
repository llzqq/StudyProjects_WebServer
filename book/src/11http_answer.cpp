#include "11http_answer.hpp"

/**
 * @brief 使用有限状态机的方法，实现http应答
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char *argv[]) {
  if (argc <= 2) {
    printf("usage:%s ip_address port_number\n", basename(argv[0]));
    return 1;
  }
  const char *ip = argv[1];
  int port = atoi(argv[2]);
  struct sockaddr_in address;
  bzero(&address, sizeof(address));
  address.sin_family = AF_INET;
  inet_pton(AF_INET, ip, &address.sin_addr);
  address.sin_port = htons(port);
  int listenfd = socket(PF_INET, SOCK_STREAM, 0);
  assert(listenfd >= 0);
  int ret = bind(listenfd, (struct sockaddr *)&address, sizeof(address));
  assert(ret != -1);
  ret = listen(listenfd, 5);
  assert(ret != -1);
  struct sockaddr_in client_address;
  socklen_t client_addrlength = sizeof(client_address);
  int fd =
      accept(listenfd, (struct sockaddr *)&client_address, &client_addrlength);

  if (fd < 0) {
    printf("errno is:%d\n", errno);
  } else {
    char buffer[BUFFER_SIZE]; /*读缓冲区*/
    memset(buffer, '\0', BUFFER_SIZE);
    int data_read = 0;
    int read_index = 0;    /*当前已经读取了多少字节的客户数据*/
    int checked_index = 0; /*当前已经分析完了多少字节的客户数据*/
    int start_line = 0;    /*行在buffer中的起始位置*/
    /*设置主状态机的初始状态*/
    CHECK_STATE checkstate = CHECK_STATE_REQUESTLINE;

    while (1) /*循环读取客户数据并分析之*/
    {
      data_read = recv(fd, buffer + read_index, BUFFER_SIZE - read_index, 0);
      if (data_read == -1) {
        printf("reading failed\n");
        break;
      } else if (data_read == 0) {
        printf("remote client has closed the connection\n");
        break;
      }
      read_index += data_read;

      /*分析目前已经获得的所有客户数据*/
      HTTP_CODE result = parse_content(buffer, checked_index, checkstate,
                                       read_index, start_line);

      if (result == NO_REQUEST) /*尚未得到一个完整的HTTP请求*/
      {
        continue;
      } else if (result == GET_REQUEST) /*得到一个完整的、正确的HTTP请求*/
      {
        send(fd, szret[0], strlen(szret[0]), 0);
        break;
      } else /*其他情况表示发生错误*/
      {
        send(fd, szret[1], strlen(szret[1]), 0);
        break;
      }
    }
    close(fd);
  }
  close(listenfd);
  return 0;
}