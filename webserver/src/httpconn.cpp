#include <webserver/httpconn.h>

namespace webserver {
const char* HttpConn::srcDir;
std::atomic<int> HttpConn::userCount;
bool HttpConn::isET;

HttpConn::HttpConn() : m_fd(-1), is_close(true) {
  m_addr = {0};
}

HttpConn::~HttpConn() {
  Close();
}

void HttpConn::init(int sockFd, const sockaddr_in &addr) {
  assert(sockFd > 0);
  userCount++;
  m_addr = addr;
  m_fd = sockFd;
  m_writebuff.retrieveall();
  m_readbuff.retrieveall();
  is_close = false;
  //LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
}

ssize_t HttpConn::read(int *saveErrno) {
  ssize_t len = -1;
  do {
    len = m_readbuff.readfd(m_fd, saveErrno);
    if(len <= 0) break;
  } while(isET);
  return len;
}

ssize_t HttpConn::write(int *saveErrno) {
  ssize_t len = -1;
  do {
    len = writev(m_fd, m_iov, m_iovcnt);
    if(len <= 0) {
      *saveErrno = errno;
      break;
    }
    if(m_iov[0].iov_len + m_iov[1].iov_len == 0) break;
    else if(static_cast<size_t>(len) > m_iov[0].iov_len) {
      m_iov[1].iov_base = (uint8_t*) m_iov[1].iov_base + (len - m_iov[0].iov_len);
      m_iov[1].iov_len -= (len - m_iov[0].iov_len);
      if(m_iov[0].iov_len) {
        m_writebuff.retrieveall();
        m_iov[0].iov_len = 0;
      }
    } else {
      m_iov[0].iov_base = (uint8_t*)m_iov[0].iov_base + len;
      m_iov[0].iov_len -= len;
      m_writebuff.retrieve(len);
    }
  } while(isET || towritebytes() > 10240);
  return len;
}

void HttpConn::Close() {
  m_response.unmapfile();
  if(!is_close) {
    is_close = true;
    userCount--;
    close(m_fd);
    //LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
  }
}

int HttpConn::getfd() const {
  return m_fd;
}

int HttpConn::getport() const {
  return m_addr.sin_port;
}

const char *HttpConn::getip() const {
  return inet_ntoa(m_addr.sin_addr);
}

sockaddr_in HttpConn::getaddr() const {
  return m_addr;
}

bool HttpConn::process() {
  m_request.init();
  if(m_readbuff.readable_bytes() <= 0) return false;
  else if(m_request.parse(m_readbuff)) {
    //LOG_DEBUG("%s", request_.path().c_str());
    m_response.init(srcDir, m_request.path(), m_request.is_keepalive(), 200);
  } else {
    puts(m_request.path().c_str());
    m_response.init(srcDir, m_request.path(), false, 400);
  }
  m_response.makeresponse(m_writebuff);
  m_iov[0].iov_base = const_cast<char*>(m_writebuff.peek());
  m_iov[0].iov_len = m_writebuff.readable_bytes();
  m_iovcnt = 1;

  if(m_response.filelen() > 0 && m_response.file()) {
    m_iov[1].iov_base = m_response.file();
    m_iov[1].iov_len = m_response.filelen();
    m_iovcnt = 2;
  }
  //LOG_DEBUG("filesize:%d, %d  to %d", response_.FileLen() , iovCnt_, ToWriteBytes());
  return true;
}

} // namespace webserver