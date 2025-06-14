#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL_

#include "../lock/locker.h"
#include <error.h>
#include <iostream>
#include <list>
#include <mysql/mysql.h>
#include <stdio.h>
#include <string.h>
#include <string>

using namespace std;

// 创建服务器与mysql的连接池
class connection_pool {
public:
  MYSQL *GetConnection();              //获取数据库连接
  bool ReleaseConnection(MYSQL *conn); //释放连接
  int GetFreeConn();                   //获取连接
  void DestroyPool();                  //销毁所有连接

  //单例模式
  static connection_pool *GetInstance();

  void init(string url, string User, string PassWord, string DataBaseName,
            int Port, unsigned int MaxConn);

  connection_pool();
  ~connection_pool();

private:
  unsigned int MaxConn;  //最大连接数
  unsigned int CurConn;  //当前已使用的连接数
  unsigned int FreeConn; //当前空闲的连接数

private:
  locker lock;
  list<MYSQL *> connList; //连接池
  sem reserve;

private:
  string url;          //主机地址
  string Port;         //数据库端口号
  string User;         //登陆数据库用户名
  string PassWord;     //登陆数据库密码
  string DatabaseName; //使用数据库名
};

// Resource Acquisition Is Initialization（资源获取即初始化）
// 在对象构造时获取资源，在对象析构时自动释放资源
// 当服务器需要一个与mysql的连接时，负责提供准备好的资源
class connectionRAII {
public:
  connectionRAII(MYSQL **con, connection_pool *connPool);
  ~connectionRAII();

private:
  MYSQL *conRAII;
  connection_pool *poolRAII;
};

#endif
