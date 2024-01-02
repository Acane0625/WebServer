#pragma once
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <webserver/buffer.h>
#include <webserver/httprequest.h>
#include <webserver/httpresponse.h>
#include <webserver/sqlconnRAII.h>

namespace webserver {
class HttpConn {
public:
  HttpConn();

  ~HttpConn();

  void init(int sockFd, const sockaddr_in &addr);

  ssize_t read(int *saveErrno);

  ssize_t write(int *saveErrno);

  void Close();

  int getfd() const;

  int getport() const;

  const char *getip() const;

  sockaddr_in getaddr() const;

  bool process();

  int towritebytes() { return m_iov[0].iov_len + m_iov[1].iov_len; }

  bool is_keepalive() const { return m_request.is_keepalive(); }

  static bool isET;
  static const char *srcDir;
  static std::atomic<int> userCount;

private:
  int m_fd;
  struct sockaddr_in m_addr;

  bool is_close;

  int m_iovcnt;
  struct iovec m_iov[2];

  Buffer m_readbuff;  // 读缓冲区
  Buffer m_writebuff; // 写缓冲区

  HttpRequest m_request;
  HttpResponse m_response;
};

} // namespace webserver