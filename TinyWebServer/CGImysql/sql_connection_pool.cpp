#include "sql_connection_pool.h"
#include <iostream>
#include <list>
#include <mysql/mysql.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

using namespace std;

connection_pool::connection_pool() {
  this->CurConn = 0;
  this->FreeConn = 0;
}

connection_pool *connection_pool::GetInstance() {
  static connection_pool connPool;
  return &connPool;
}

//构造初始化
void connection_pool::init(string url, string User, string PassWord,
                           string DBName, int Port, unsigned int MaxConn) {
  this->url = url;
  this->Port = Port;
  this->User = User;
  this->PassWord = PassWord;
  this->DatabaseName = DBName;

  /*批量创建和初始化MySQL数据库连接 */
  lock.lock();
  for (int i = 0; i < MaxConn; i++) {
    MYSQL *con = NULL;
    con = mysql_init(con); // 分配内存，返回 MYSQL 结构体的指针
    if (con == NULL) {
      cout << "Error:" << mysql_error(con);
      exit(1);
    }

    con = mysql_real_connect(con, url.c_str(), User.c_str(), PassWord.c_str(),
                             DBName.c_str(), Port, NULL, 0); // 建立数据库连接
    if (con == NULL) {
      cout << "Error: " << mysql_error(con);
      exit(1);
    }

    connList.push_back(con); // 将成功创建的连接加入连接池列表
    ++FreeConn;              // 增加空闲连接计数器
  }

  reserve = sem(FreeConn);  // 初始化信号量
  this->MaxConn = FreeConn; // 设置最大连接数

  lock.unlock();
}

//当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
MYSQL *connection_pool::GetConnection() {
  MYSQL *con = NULL;

  if (0 == connList.size())
    return NULL;

  reserve.wait(); // 信号量减 1

  lock.lock();

  con = connList.front(); // 从连接池拿出最前面的给当前连接
  connList.pop_front();   // 并且从连接池中去除
  --FreeConn;
  ++CurConn;

  lock.unlock();
  return con;
}

//释放当前使用的连接
bool connection_pool::ReleaseConnection(MYSQL *con) {
  if (NULL == con)
    return false;

  lock.lock();

  connList.push_back(con);
  ++FreeConn;
  --CurConn;

  lock.unlock();

  reserve.post();
  return true;
}

//销毁数据库连接池
void connection_pool::DestroyPool() {

  lock.lock();
  if (connList.size() > 0) {
    list<MYSQL *>::iterator it;
    for (it = connList.begin(); it != connList.end(); ++it) {
      MYSQL *con = *it;
      mysql_close(con);
    }
    CurConn = 0;
    FreeConn = 0;
    connList.clear();

    lock.unlock();
  }

  lock.unlock();
}

//当前空闲的连接数
int connection_pool::GetFreeConn() { return this->FreeConn; }

connection_pool::~connection_pool() { DestroyPool(); }

connectionRAII::connectionRAII(MYSQL **SQL, connection_pool *connPool) {
  *SQL = connPool->GetConnection();

  conRAII = *SQL;
  poolRAII = connPool;
}

connectionRAII::~connectionRAII() { poolRAII->ReleaseConnection(conRAII); }