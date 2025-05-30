#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "../CGImysql/sql_connection_pool.h"
#include "../lock/locker.h"
#include <cstdio>
#include <exception>
#include <list>
#include <pthread.h>

// 负责创建客户端与服务器的连接池
// 在 run 函数中，真正为客户端创建 客户端--服务器--mysql 的连接
template <typename T> class threadpool {
public:
  /*thread_number是线程池中线程的数量，max_requests是请求队列中最多允许的、等待处理的请求的数量*/
  threadpool(connection_pool *connPool, int thread_number = 8,
             int max_request = 10000);
  ~threadpool();
  bool append(T *request);

private:
  /*工作线程运行的函数，它不断从工作队列中取出任务并执行之*/
  static void *worker(void *arg);
  void run();

private:
  int m_thread_number;  //线程池中的线程数
  int m_max_requests;   //请求队列中允许的最大请求数
  pthread_t *m_threads; //描述线程池的数组，其大小为m_thread_number
  std::list<T *> m_workqueue;  //请求队列
  locker m_queuelocker;        //保护请求队列的互斥锁
  sem m_queuestat;             //信号量，表示是否有任务需要处理
  bool m_stop;                 //是否结束线程
  connection_pool *m_connPool; //数据库
};

template <typename T>
threadpool<T>::threadpool(connection_pool *connPool, int thread_number,
                          int max_requests)
    : m_thread_number(thread_number), m_max_requests(max_requests),
      m_stop(false), m_threads(NULL), m_connPool(connPool) {
  if (thread_number <= 0 || max_requests <= 0)
    throw std::exception();

  m_threads = new pthread_t[m_thread_number]; // 创建 pthread_t 类型的数组
                                              // 并且分配大小为m_thread_number
                                              // 但是还未创建对象（线程）
  if (!m_threads)
    throw std::exception();
  for (int i = 0; i < thread_number; ++i) {
    // pthread_create 函数用于创建一个新线程。新线程立即从
    // __start_routine（worker）函数开始执行，并传递 __arg（this）
    // 作为参数；线程的属性由 __attr（NULL） 指定，新线程的句柄存储在
    // __newthread （m_threads + i）中
    if (pthread_create(m_threads + i, NULL, worker, this) != 0) {
      delete[] m_threads;
      throw std::exception();
    }
    if (pthread_detach(m_threads[i])) {
      delete[] m_threads;
      throw std::exception();
    }
  }
}

template <typename T> threadpool<T>::~threadpool() {
  delete[] m_threads;
  m_stop = true;
}

template <typename T> bool threadpool<T>::append(T *request) {
  m_queuelocker.lock();
  if (m_workqueue.size() > m_max_requests) {
    m_queuelocker.unlock();
    return false;
  }
  m_workqueue.push_back(request);
  m_queuelocker.unlock();
  m_queuestat.post();
  return true;
}

template <typename T> void *threadpool<T>::worker(void *arg) {
  threadpool *pool = (threadpool *)arg; // arg 传入的是 main.cpp 中的pool
  pool->run(); // 可以看出多线程共享对象等资源
  return pool;
}

template <typename T> void threadpool<T>::run() {
  while (!m_stop) {
    m_queuestat.wait(); // 阻塞的线程会被操作系统挂起，进入等待状态。
                        // 挂起的线程不会占用 CPU 时间，也不会执行任何代码。
    m_queuelocker.lock(); // 所以不存在竞争，下面才存在竞争
    if (m_workqueue.empty()) {
      m_queuelocker.unlock();
      continue; // 跳过后续代码，进入下一次循环
    }
    T *request = m_workqueue.front(); // 请求队列不为空时，处理队首的连接
    m_workqueue.pop_front();
    m_queuelocker.unlock();
    if (!request)
      continue;

    /*客户端 <---> 服务器 <---> mysql数据库
        thread_pool     connection_pool
    从 m_connPool 取出一个服务器与mysql的连接，给 request->mysql
     */
    connectionRAII mysqlcon(&request->mysql, m_connPool);

    request->process();
  }
}
#endif
