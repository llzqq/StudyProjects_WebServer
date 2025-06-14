#ifndef LOG_H
#define LOG_H

#include "block_queue.h"
#include <iostream>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string>

using namespace std;

class Log {
public:
  // C++11以后,使用局部变量懒汉不用加锁
  // 单例式设计
  // 不需要实例化类的对象就能调用该方法
  static Log *get_instance() // 返回类型为Log指针，类内的静态函数
  {
    /*创建作用域为局部，但是生命贯穿程序的静态变量
     同时相当于创建了实例，可以调用各种类的方法
     通过调用get_instance() 就可以访问该实例*/
    static Log instance;
    return &instance;
  }

  static void *flush_log_thread(void *args) {
    Log::get_instance()->async_write_log();
  }

  //可选择的参数有日志文件、日志缓冲区大小、最大行数以及最长日志条队列
  bool init(const char *file_name, int log_buf_size = 8192,
            int split_lines = 5000000, int max_queue_size = 0);

  void write_log(int level, const char *format, ...);

  // 强制将日志缓冲区的内容写入磁盘文件，类似 std::endl
  void flush(void);

private:
  Log();
  virtual ~Log();

  void *async_write_log() { // 异步 asynchronous；同步 synchronous
    string single_log;
    //从阻塞队列中取出一个日志string，写入文件
    while (m_log_queue->pop(single_log)) {
      m_mutex.lock();
      fputs(single_log.c_str(), m_fp);
      m_mutex.unlock();
    }
  }

private:
  char dir_name[128]; //路径名
  char log_name[128]; // log文件名
  int m_split_lines;  //日志最大行数
  int m_log_buf_size; //日志缓冲区大小
  long long m_count;  //日志行数记录
  int m_today;        //因为按天分类,记录当前时间是那一天
  FILE *m_fp;         //打开log的文件指针
  char *m_buf;        // 缓冲区实际保存数据的数组
  block_queue<string> *m_log_queue; //阻塞队列
  bool m_is_async;                  //是否同步标志位
  locker m_mutex;
};

#define LOG_DEBUG(format, ...)                                                 \
  Log::get_instance()->write_log(0, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...)                                                  \
  Log::get_instance()->write_log(1, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...)                                                  \
  Log::get_instance()->write_log(2, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...)                                                 \
  Log::get_instance()->write_log(3, format, ##__VA_ARGS__)

#endif
