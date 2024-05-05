
#ifndef SQLCONNRAII_H
#define SQLCONNRAII_H
#include "sqlconnpool.h"
#include <cassert>
#include <mysql/mysql.h>

class SqlConnRAII {
public:
  SqlConnRAII(MYSQL **sql, SqlConnPool *connPool) {
    assert(connPool);
    //不能直接将指针赋值给另一个指针，因为指针本身就是一个值，而不是一个引用。
    *sql = connPool->GetConn();
    sql_ = *sql;
    connpool_ = connPool;
  }
  ~SqlConnRAII() {
    if (sql_) {
      connpool_->FreeConn(sql_);
    }
  }

private:
  MYSQL *sql_;
  SqlConnPool *connpool_;
};

#endif