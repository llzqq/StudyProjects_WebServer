#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

// 信号量
class sem {
public:
  sem() {
    if (sem_init(&m_sem, 0, 0) != 0) {
      throw std::exception(); //设置错误处理机制，如果进了 if
                              //会停止程序。需要在调用的地方try-catch
    }
  }
  sem(int num) {
    if (sem_init(&m_sem, 0, num) != 0) {
      throw std::exception();
    }
  }
  ~sem() { sem_destroy(&m_sem); }

  // 尝试将信号量值减1；如果当前值>0，减1后继续执行；如果当前值=0，线程阻塞直到值>0
  bool wait() { return sem_wait(&m_sem) == 0; }

  // 将信号量值加1；如果有线程因wait操作阻塞，会唤醒其中一个
  bool post() { return sem_post(&m_sem) == 0; }

private:
  sem_t m_sem;
};

// 对 mutex锁 重命名，方便调用
class locker {
public:
  locker() {
    if (pthread_mutex_init(&m_mutex, NULL) != 0) {
      throw std::exception();
    }
  }
  ~locker() { pthread_mutex_destroy(&m_mutex); }

  /*如果锁可用：函数立即返回，线程获得锁
    如果锁被占用：线程会被阻塞，直到获得锁后函数才返回
  */
  bool lock() { return pthread_mutex_lock(&m_mutex) == 0; }
  bool unlock() { return pthread_mutex_unlock(&m_mutex) == 0; }
  pthread_mutex_t *get() { return &m_mutex; }

private:
  pthread_mutex_t m_mutex; // mutex ：互斥锁
};

// 用于实现线程间的等待/通知机制
class cond {
public:
  cond() {
    if (pthread_cond_init(&m_cond, NULL) != 0) {
      // pthread_mutex_destroy(&m_mutex);
      throw std::exception();
    }
  }

  ~cond() { pthread_cond_destroy(&m_cond); }

  // 原子地释放互斥锁并使线程挂起；当被唤醒时，重新竞争互斥锁后返回
  // 阻塞函数，直到被唤醒
  bool wait(pthread_mutex_t *m_mutex) {
    int ret = 0;
    // pthread_mutex_lock(&m_mutex);
    ret = pthread_cond_wait(&m_cond, m_mutex);
    // pthread_mutex_unlock(&m_mutex);
    return ret == 0;
  }

  // 与wait类似，但增加了超时参数；如果超过指定时间仍未被唤醒，会返回错误码
  bool timewait(pthread_mutex_t *m_mutex, struct timespec t) {
    int ret = 0;
    // pthread_mutex_lock(&m_mutex);
    ret = pthread_cond_timedwait(&m_cond, m_mutex, &t);
    // pthread_mutex_unlock(&m_mutex);
    return ret == 0;
  }
  // 唤醒一个等待该条件的线程
  bool signal() { return pthread_cond_signal(&m_cond) == 0; }

  // 唤醒所有等待该条件的线程
  bool broadcast() { return pthread_cond_broadcast(&m_cond) == 0; }

private:
  // static pthread_mutex_t m_mutex;
  pthread_cond_t m_cond;
};
#endif
