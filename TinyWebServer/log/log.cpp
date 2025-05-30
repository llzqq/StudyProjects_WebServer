#include "log.h"
#include <pthread.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
using namespace std;

Log::Log() {
  m_count = 0;
  m_is_async = false;
}

Log::~Log() {
  if (m_fp != NULL) {
    fclose(m_fp);
  }
}

/**
 * @brief 异步需要设置阻塞队列的长度，同步不需要设置
 *
 * @param file_name
 * @param log_buf_size
 * @param split_lines
 * @param max_queue_size
 */
bool Log::init(const char *file_name, int log_buf_size, int split_lines,
               int max_queue_size) {
  //如果设置了max_queue_size,则设置为异步，创建一个线程专门用来写入日志
  if (max_queue_size >= 1) {
    m_is_async = true;
    m_log_queue = new block_queue<string>(max_queue_size);
    pthread_t tid;
    // flush_log_thread为回调函数,这里表示创建线程异步写日志
    // 在这里调用了 pop ，不断写入日志
    pthread_create(&tid, NULL, flush_log_thread, NULL);
  }

  m_log_buf_size = log_buf_size;
  m_buf = new char[m_log_buf_size];
  memset(m_buf, '\0', m_log_buf_size);
  m_split_lines = split_lines;

  /*初始化时间变量*/
  time_t t = time(NULL);
  struct tm *sys_tm = localtime(&t);
  struct tm my_tm = *sys_tm;

  /*确定文件名 log_full_name*/
  const char *p = strrchr(file_name, '/'); // 检查指定的文件名，是否包含路径
  char log_full_name[256] = {0};

  if (p == NULL) { // 如果 file_name 不含 '/'，即没有指定地址，只指定了文件名
    snprintf(log_full_name, 255, "%d_%02d_%02d_%s", my_tm.tm_year + 1900,
             my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
  } else { // 如果file_name 含地址，将文件名和地址拆开
    strcpy(log_name, p + 1); // log_name是指定的文件名
    // p指向文件名后面，file_name指向头部，减出来得到正整数，dir_name就是纯粹的地址
    strncpy(dir_name, file_name, p - file_name + 1);
    snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", dir_name,
             my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name);
  }

  /*my_tm结构体数据只初始化一次，但是每天都要记录新的日志，所以根据
   * m_today判断是否要打开新的日志文件*/
  m_today = my_tm.tm_mday;

  m_fp = fopen(log_full_name, "a"); // a参数含义：每次写操作时都为追加写
  if (m_fp == NULL) {
    printf("failed to open file\n");
    return false;
  }

  return true;
}

void Log::write_log(int level, const char *format, ...) {
  /*设置时间变量*/
  struct timeval now = {0, 0};
  gettimeofday(&now, NULL);
  time_t t = now.tv_sec;
  struct tm *sys_tm = localtime(&t);
  struct tm my_tm = *sys_tm;

  /*设置消息头*/
  char s[16] = {0};
  switch (level) {
  case 0:
    strcpy(s, "[debug]:");
    break;
  case 1:
    strcpy(s, "[info]:");
    break;
  case 2:
    strcpy(s, "[warn]:");
    break;
  case 3:
    strcpy(s, "[erro]:");
    break;
  default:
    strcpy(s, "[info]:");
    break;
  }

  //写入一个log，对m_count++, m_split_lines最大行数
  m_mutex.lock();
  m_count++;

  /*新的一天，或者是写满了时：打开文件名为 new_log 的新日志*/
  if (m_today != my_tm.tm_mday || m_count % m_split_lines == 0) // everyday log
  {

    char new_log[256] = {0};
    fflush(m_fp);
    fclose(m_fp);
    char tail[16] = {0};

    snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1,
             my_tm.tm_mday);

    // TODO 被我发现bug了
    if (m_today != my_tm.tm_mday) {
      snprintf(new_log, 255, "%s%s%s", dir_name, tail, log_name);
      m_today = my_tm.tm_mday;
      m_count = 0;
    } else {
      snprintf(new_log, 255, "%s%s%s.%lld", dir_name, tail, log_name,
               m_count / m_split_lines);
    }
    m_fp = fopen(new_log, "a");
  }

  m_mutex.unlock();

  /*日志内容格式化
  时间和输入的字符串*/
  va_list valst;
  va_start(valst, format);
  string log_str;
  m_mutex.lock();

  // 写入的具体时间 内容格式，每行的开头
  int n = snprintf(m_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                   my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                   my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec,
                   s); // 每次写入都会覆盖原有内容

  int m = vsnprintf(m_buf + n, m_log_buf_size - 1, format,
                    valst); // 写入 字符串内容
  m_buf[n + m] = '\n';
  m_buf[n + m + 1] = '\0';
  log_str = m_buf;

  m_mutex.unlock();

  if (m_is_async &&
      !m_log_queue->full()) { // 异步模式下，将日志放入阻塞队列，由后台线程处理
    m_log_queue->push(log_str);
  } else { // 当异步模式下队列已满或同步模式下，直接写入文件
    m_mutex.lock();
    fputs(log_str.c_str(), m_fp);
    m_mutex.unlock();
  }

  va_end(valst);
}

void Log::flush(void) {
  m_mutex.lock();
  //强制刷新写入流缓冲区
  fflush(m_fp);
  m_mutex.unlock();
}
