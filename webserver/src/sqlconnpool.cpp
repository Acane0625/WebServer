#include <webserver/sqlconnpool.h>

namespace webserver {

SqlConnPool::SqlConnPool() : m_usecount(0), m_freecount(0) {}

SqlConnPool::~SqlConnPool() {
  ClosePool();
}

SqlConnPool &SqlConnPool::Instance() {
  static SqlConnPool connpool;
  return connpool;
}

MYSQL *SqlConnPool::GetConn() {
  MYSQL *sql = nullptr;
  if(m_connque.empty()) {
    //LOG_WARN("SqlConnPool busy!");
    return nullptr;
  }
  sem_wait(&m_semid);
  {
    std::lock_guard<std::mutex> lock(m_mtx);
    sql = m_connque.front();
    m_connque.pop();
  }
  return sql;
}

void SqlConnPool::FreeConn(MYSQL *sql) {
  assert(sql);
  std::lock_guard<std::mutex> lock(m_mtx);
  m_connque.push(sql);
  sem_post(&m_semid);
}

int SqlConnPool::GetFreeConnCount() {
  std::lock_guard<std::mutex> lock(m_mtx);
  return m_connque.size();
}

void SqlConnPool::Init(const char *host, int port, const char *user, const char *pwd, const char *dbName, int connSize = 10) {
  assert(connSize > 0);
  for(int i = 0; i < connSize; i++) {
    MYSQL *sql = nullptr;
    sql = mysql_init(sql);
    if(!sql) {
      //LOG_ERROR("MySql init error!");
      assert(sql);
    }
    sql = mysql_real_connect(sql, host, user, pwd, dbName, port, nullptr, 0);
    if(!sql) {
      //LOG_ERROR("MySql Connect error!");
    }    
    m_connque.push(sql);
  }
  MAX_CONN = connSize;
  sem_init(&m_semid, 0, MAX_CONN);
}

void SqlConnPool::ClosePool() {
  std::lock_guard<std::mutex> lock(m_mtx);
  while(!m_connque.empty()) {
    auto item = m_connque.front();
    m_connque.pop();
    mysql_close(item);
  }
  mysql_library_end();
}

} // namespace webserver