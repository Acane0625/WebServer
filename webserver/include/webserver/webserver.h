#pragma once
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h> // fcntl()
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h> // close()
#include <unordered_map>
#include <webserver/epoller.h>
#include <webserver/heaptimer.h>
#include <webserver/httpconn.h>
#include <webserver/sqlconnRAII.h>
#include <webserver/sqlconnpool.h>
#include <webserver/threadpool.h>

namespace webserver {
class WebServer {
public:
  WebServer(int port, int trigMode, int timeoutMS, bool OptLinger, int sqlPort,
            const char *sqlUser, const char *sqlPwd, const char *dbName,
            int connPoolNum, int threadNum, bool openLog, int logLevel,
            int logQueSize);

  ~WebServer();
  void start();

private:
  bool init_socket();
  void init_eventmode(int trigMode);
  void add_client(int fd, sockaddr_in addr);

  void deal_listen();
  void deal_write(HttpConn *client);
  void deal_read(HttpConn *client);

  void send_error(int fd, const char *info);
  void extent_time(HttpConn *client);
  void close_conn(HttpConn *client);

  void on_read(HttpConn *client);
  void on_write(HttpConn *client);
  void on_process(HttpConn *client);

  static const int MAX_FD = 65536;

  static int setfd_nonblock(int fd);

  int m_port;
  bool m_openlinger;
  int m_timeoutMS; /* 毫秒MS */
  bool m_isClose;
  int m_listenfd;
  char *m_srcdir;

  uint32_t m_listen_event;
  uint32_t m_conn_event;

  std::unique_ptr<HeapTimer> m_timer;
  std::unique_ptr<ThreadPool> m_threadpool;
  std::unique_ptr<Epoller> m_epoller;
  std::unordered_map<int, HttpConn> m_users;
};

} // namespace webserver