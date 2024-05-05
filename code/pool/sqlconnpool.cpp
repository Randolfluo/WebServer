
#include "sqlconnpool.h"
#include <cassert>
#include <cstddef>
#include <mutex>
#include <mysql/mysql.h>
#include <semaphore.h>

SqlConnPool *SqlConnPool::Instance() {
  static SqlConnPool connPool;
  return &connPool;
}

MYSQL *SqlConnPool::GetConn() {
  MYSQL *sql = nullptr;
  if (connQue_.empty()) {
    LOG_ERROR("SqlConnPool busy!");
    return nullptr;
  }
  sem_wait(&semId_);
  {
    std::lock_guard<std::mutex> locker(mtx_);
    sql = connQue_.front();
    connQue_.pop();
  }
  return sql;
}
void SqlConnPool::FreeConn(MYSQL *conn) {
  assert(conn);
  std::lock_guard<std::mutex> locker(mtx_);
  connQue_.push(conn);
  sem_post(&semId_);
}
int SqlConnPool::GetFreeConnCount() {
  std::lock_guard<std::mutex> locker(mtx_);
  return connQue_.size();
}
void SqlConnPool::Init(const char *host, int port, const char *user,
                       const char *pwd, const char *dbName, int connSize = 10) {
  assert(connSize > 0);
  for (int i = 0; i < connSize; i++) {
    MYSQL *sql = nullptr;
    sql = mysql_init(sql);
    if (!sql) {
      LOG_ERROR("MySql init error!");
      assert(sql); //因为无法初始化就无法正常执行数据库连接，因此需要输出错误位置结束程序
    }
    sql = mysql_real_connect(sql, host, user, pwd, dbName, port, nullptr, 0);
    if (!sql) {
      LOG_ERROR("MySql Connect error!");
      //assert(sql);  第一次知道断言是在运行时执行的，与 if 语句的控制流无关。 (_Qwq_)
    }
    connQue_.push(sql);
  }
  MAX_CONN_ = connSize;
  sem_init(&semId_, 0, MAX_CONN_);
}
void SqlConnPool::ClosePool() {
  std::lock_guard<std::mutex> locker(mtx_);
  while (!connQue_.empty()) {
    auto item = connQue_.front();
    connQue_.pop();
    mysql_close(item);
  }
  mysql_library_end(); //释放mysql资源，防止内存泄露
}

SqlConnPool::SqlConnPool() : useCount_(0), freeCount_(0) {}

SqlConnPool::~SqlConnPool() { ClosePool(); }
