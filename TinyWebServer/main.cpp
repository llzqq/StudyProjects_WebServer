#include <arpa/inet.h>
#include <cassert>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "./CGImysql/sql_connection_pool.h"
#include "./http/http_conn.h"
#include "./lock/locker.h"
#include "./log/log.h"
#include "./threadpool/threadpool.h"
#include "./timer/lst_timer.h"

#define MAX_FD 65536           //最大文件描述符
#define MAX_EVENT_NUMBER 10000 //最大事件数
#define TIMESLOT 5             //最小超时单位

#define SYNLOG //同步写日志
//#define ASYNLOG //异步写日志

//#define listenfdET //边缘触发非阻塞
#define listenfdLT //水平触发阻塞

//这三个函数在http_conn.cpp中定义，改变链接属性
extern int addfd(int epollfd, int fd, bool one_shot);
extern int removefd(int epollfd, int fd);
extern int setnonblocking(int fd);

//设置定时器相关参数
static int pipefd[2];
static sort_timer_lst timer_lst;
static int epollfd = 0;

//信号处理函数
void sig_handler(int sig) {
  //为保证函数的可重入性，保留原来的errno
  int save_errno = errno;
  int msg = sig;
  send(pipefd[1], (char *)&msg, 1, 0);
  errno = save_errno;
}

/**
 * @brief 封装 sigaction
 *
 * @param sig 要处理的信号类型如SIGPIPE、SIGALRM、SIGTERM等
 *
 * @param handler 信号处理函数指针；
 * 可以是自定义函数如sig_handler；
 * 也可以是系统预定义的如SIG_IGN（忽略信号）
 *
 * @param restart 是否设置自动重启标志；
 * true：被信号中断的系统调用会自动重启；
 * false：被信号中断的系统调用直接返回错误
 */
void addsig(int sig, void(handler)(int), bool restart = true) {
  struct sigaction sa;
  memset(&sa, '\0', sizeof(sa));
  sa.sa_handler = handler;
  if (restart)
    sa.sa_flags |= SA_RESTART;
  //在处理当前信号期间，阻塞所有其他信号；确保信号处理的原子性，避免信号处理函数被其他信号中断
  sigfillset(&sa.sa_mask);
  // sigaction 注册成功时返回0，失败则返回-1并设置errno。
  // assert 条件为真：程序继续正常执行；条件为假：程序立即终止，并输出错误信息
  assert(sigaction(sig, &sa, NULL) != -1);
}

//定时处理任务，重新定时以不断触发SIGALRM信号
void timer_handler() {
  timer_lst.tick();
  alarm(TIMESLOT);
}

//定时器回调函数，删除非活动连接在socket上的注册事件，并关闭
void cb_func(client_data *user_data) {
  epoll_ctl(epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
  assert(user_data);
  close(user_data->sockfd);
  http_conn::m_user_count--;
  LOG_INFO("close fd %d", user_data->sockfd);
  Log::get_instance()->flush();
}

void show_error(int connfd, const char *info) {
  printf("%s", info);
  send(connfd, info, strlen(info), 0);
  close(connfd);
}

int main(int argc, char *argv[]) {
#ifdef ASYNLOG
  Log::get_instance()->init("ServerLog", 2000, 800000, 8); //异步日志模型
#endif

#ifdef SYNLOG
  Log::get_instance()->init("ServerLog", 2000, 800000, 0); //同步日志模型
#endif

  if (argc <= 1) {
    printf("usage: %s ip_address port_number\n", basename(argv[0]));
    return 1;
  }

  int port = atoi(argv[1]);

  // 忽略SIGPIPE信号
  // SIGPIPE：当向已关闭的socket写数据时产生
  // 忽略此信号避免服务器因客户端异常断开而崩溃
  addsig(SIGPIPE, SIG_IGN);

  //创建数据库连接池
  connection_pool *connPool = connection_pool::GetInstance();
  connPool->init("localhost", "lzq", "lzq", "lzqdb", 3306, 8);

  //创建线程池
  threadpool<http_conn> *pool = NULL; // 只声明了一个变量
  try {
    pool = new threadpool<http_conn>(connPool); // 这里才分配内存，创建了对象
  } catch (...) {
    return 1;
  }

  http_conn *users = new http_conn[MAX_FD];
  assert(users);

  //初始化数据库读取表，方便后续执行注册与登陆操作
  users->initmysql_result(connPool);

  /*初始化监听 socket*/
  int listenfd = socket(PF_INET, SOCK_STREAM, 0);
  assert(listenfd >= 0);

  // struct linger tmp={1,0};
  // SO_LINGER若有数据待发送，延迟关闭
  // setsockopt(listenfd,SOL_SOCKET,SO_LINGER,&tmp,sizeof(tmp));

  int ret = 0;
  struct sockaddr_in address;
  bzero(&address, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr =
      htonl(INADDR_ANY); // 监听本机上所有可用的网络接口的入站连接请求
  address.sin_port = htons(port);

  int flag = 1;
  setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
  ret = bind(listenfd, (struct sockaddr *)&address, sizeof(address));
  assert(ret >= 0);
  ret = listen(listenfd, 5);
  assert(ret >= 0);

  //创建内核事件表
  epoll_event events[MAX_EVENT_NUMBER];
  epollfd = epoll_create(5);
  assert(epollfd != -1);

  addfd(epollfd, listenfd, false); // 将 listenfd 注册到 epollfd内核事件表
  http_conn::m_epollfd = epollfd;

  //创建管道
  ret =
      socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd); // int pipefd[2]是声明
                                                   //需要函数赋予功能后才能读写
  assert(ret != -1);
  setnonblocking(pipefd[1]);
  addfd(epollfd, pipefd[0], false);

  addsig(SIGALRM, sig_handler, false);
  addsig(SIGTERM, sig_handler, false);
  bool stop_server = false;

  client_data *users_timer = new client_data[MAX_FD];

  bool timeout = false;
  alarm(TIMESLOT);

  while (!stop_server) {
    int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
    if (number < 0 && errno != EINTR) {
      LOG_ERROR("%s", "epoll failure");
      break;
    }

    for (int i = 0; i < number; i++) {
      int sockfd = events[i].data.fd;

      //处理新到的客户连接
      if (sockfd == listenfd) {
        struct sockaddr_in client_address;
        socklen_t client_addrlength = sizeof(client_address);
#ifdef listenfdLT
        int connfd = accept(listenfd, (struct sockaddr *)&client_address,
                            &client_addrlength);
        if (connfd < 0) {
          LOG_ERROR("%s:errno is:%d", "accept error", errno);
          continue;
        }
        if (http_conn::m_user_count >= MAX_FD) {
          show_error(connfd, "Internal server busy");
          LOG_ERROR("%s", "Internal server busy");
          continue;
        }
        users[connfd].init(connfd, client_address);

        //初始化client_data数据
        //创建定时器，设置回调函数和超时时间，绑定用户数据，将定时器添加到链表中
        users_timer[connfd].address = client_address;
        users_timer[connfd].sockfd = connfd;
        util_timer *timer = new util_timer;
        timer->user_data = &users_timer[connfd];
        timer->cb_func = cb_func;
        time_t cur = time(NULL);
        timer->expire = cur + 3 * TIMESLOT;
        users_timer[connfd].timer = timer;
        timer_lst.add_timer(timer);
#endif

#ifdef listenfdET
        while (1) {
          int connfd = accept(listenfd, (struct sockaddr *)&client_address,
                              &client_addrlength);
          if (connfd < 0) {
            LOG_ERROR("%s:errno is:%d", "accept error", errno);
            break;
          }
          if (http_conn::m_user_count >= MAX_FD) {
            show_error(connfd, "Internal server busy");
            LOG_ERROR("%s", "Internal server busy");
            break;
          }
          users[connfd].init(connfd, client_address);

          //初始化client_data数据
          //创建定时器，设置回调函数和超时时间，绑定用户数据，将定时器添加到链表中
          users_timer[connfd].address = client_address;
          users_timer[connfd].sockfd = connfd;
          util_timer *timer = new util_timer;
          timer->user_data = &users_timer[connfd];
          timer->cb_func = cb_func;
          time_t cur = time(NULL);
          timer->expire = cur + 3 * TIMESLOT;
          users_timer[connfd].timer = timer;
          timer_lst.add_timer(timer);
        }
        continue;
#endif
      }

      else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
        //服务器端关闭连接，移除对应的定时器
        util_timer *timer = users_timer[sockfd].timer;
        timer->cb_func(&users_timer[sockfd]);

        if (timer) {
          timer_lst.del_timer(timer);
        }
      }

      //处理信号
      else if ((sockfd == pipefd[0]) && (events[i].events & EPOLLIN)) {
        int sig;
        char signals[1024];
        ret = recv(pipefd[0], signals, sizeof(signals), 0);
        if (ret == -1) {
          continue;
        } else if (ret == 0) {
          continue;
        } else {
          for (int i = 0; i < ret; ++i) {
            switch (signals[i]) {
            case SIGALRM: {
              timeout = true;
              break;
            }
            case SIGTERM: {
              stop_server = true;
            }
            }
          }
        }
      }

      //处理客户连接上接收到的数据
      else if (events[i].events & EPOLLIN) {
        util_timer *timer = users_timer[sockfd].timer;
        if (users[sockfd].read_once()) {
          LOG_INFO("deal with the client(%s)",
                   inet_ntoa(users[sockfd].get_address()->sin_addr));
          Log::get_instance()->flush();
          //若监测到读事件，将该事件放入请求队列
          pool->append(users + sockfd);

          //若有数据传输，则将定时器往后延迟3个单位
          //并对新的定时器在链表上的位置进行调整
          if (timer) {
            time_t cur = time(NULL);
            timer->expire = cur + 3 * TIMESLOT;
            LOG_INFO("%s", "adjust timer once");
            Log::get_instance()->flush();
            timer_lst.adjust_timer(timer);
          }
        } else {
          timer->cb_func(&users_timer[sockfd]);
          if (timer) {
            timer_lst.del_timer(timer);
          }
        }
      } else if (events[i].events & EPOLLOUT) {
        util_timer *timer = users_timer[sockfd].timer;
        if (users[sockfd].write()) {
          LOG_INFO("send data to the client(%s)",
                   inet_ntoa(users[sockfd].get_address()->sin_addr));
          Log::get_instance()->flush();

          //若有数据传输，则将定时器往后延迟3个单位
          //并对新的定时器在链表上的位置进行调整
          if (timer) {
            time_t cur = time(NULL);
            timer->expire = cur + 3 * TIMESLOT;
            LOG_INFO("%s", "adjust timer once");
            Log::get_instance()->flush();
            timer_lst.adjust_timer(timer);
          }
        } else {
          timer->cb_func(&users_timer[sockfd]);
          if (timer) {
            timer_lst.del_timer(timer);
          }
        }
      }
    }
    if (timeout) {
      timer_handler();
      timeout = false;
    }
  }
  close(epollfd);
  close(listenfd);
  close(pipefd[1]);
  close(pipefd[0]);
  delete[] users;
  delete[] users_timer;
  delete pool;
  return 0;
}
