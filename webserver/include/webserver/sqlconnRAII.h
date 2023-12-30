#pragma once
#include <webserver/sqlconnpool.h>

namespace webserver {
class SqlConnRAII {
public:
  SqlConnRAII(MYSQL **sql, SqlConnPool *connpool) {
    assert(connpool);
    *sql = connpool->GetConn();
    m_sql = *sql;
    m_connpool = connpool;
  }

  ~SqlConnRAII() {
    if (m_sql) {
      m_connpool->FreeConn(m_sql);
    }
  }

private:
  MYSQL *m_sql;
  SqlConnPool *m_connpool;
};
} // namespace webserver