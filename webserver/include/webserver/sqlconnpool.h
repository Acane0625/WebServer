#pragma once
#include <assert.h>
#include <mutex>
#include <mysql/mysql.h>
#include <queue>
#include <semaphore.h>
#include <string>
#include <thread>

namespace webserver {
class SqlConnPool {
private:
  SqlConnPool();
  ~SqlConnPool();
  int MAX_CONN;
  int m_usecount;
  int m_freecount;
  std::queue<MYSQL *> m_connque;
  std::mutex m_mtx;
  sem_t m_semid;

public:
  static SqlConnPool &Instance();
  SqlConnPool(const SqlConnPool &) = delete;
  SqlConnPool &operator=(const SqlConnPool &) = delete;
  MYSQL *GetConn();
  void FreeConn(MYSQL *sql);
  int GetFreeConnCount();
  void Init(const char *host, int port, const char *user, const char *pwd,
            const char *dbName, int connSize);
  void ClosePool();
};
} // namespace webserver