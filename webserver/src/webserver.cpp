#include <webserver/webserver.h>

namespace webserver {

WebServer::WebServer(int port, int trigMode, int timeoutMS, bool OptLinger,
                     int sqlPort, const char *sqlUser, const char *sqlPwd,
                     const char *dbName, int connPoolNum, int threadNum,
                     bool openLog, int logLevel, int logQueSize) : 
  m_port(port), m_openlinger(OptLinger), m_timeoutMS(timeoutMS), m_isClose(false),
  m_timer(new HeapTimer()), m_threadpool(new ThreadPool(threadNum)), m_epoller(new Epoller()) {
    m_srcdir = getcwd(nullptr, 256);
    assert(m_srcdir);
    strncat(m_srcdir, "/resources/", 16);
    HttpConn::userCount = 0;
    HttpConn::srcDir = m_srcdir;
    SqlConnPool::Instance().Init("localhost", sqlPort, sqlUser, sqlPwd, dbName, connPoolNum);
    init_eventmode(trigMode);
    if(!init_socket()) m_isClose = true;

    if(openLog) {
      /*
      */
    }
  }

WebServer::~WebServer() {
  close(m_listenfd);
  m_isClose = true;
  free(m_srcdir);
  SqlConnPool::Instance().ClosePool();
}

void WebServer::start() {
  int timeMS = -1;
  if(!m_isClose) {
    //LOG_INFO("========== Server start ==========");
  }
  while(!m_isClose) {
    if(m_timeoutMS > 0) timeMS = m_timer->getnexttick();
    int eventcnt = m_epoller->wait(timeMS);
    for(int i = 0; i < eventcnt; i++) {
      int fd = m_epoller->geteventfd(i);
      uint32_t events = m_epoller->getevents(i);
      if(fd == m_listenfd) deal_listen();
      else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
        assert(m_users.count(fd) > 0);
        close_conn(&m_users[fd]);
      }
      else if(events & EPOLLIN) {
        assert(m_users.count(fd));
        deal_read(&m_users[fd]);
      }
      else if(events & EPOLLOUT) {
        assert(m_users.count(fd));
        deal_write(&m_users[fd]);
      } 
      else {
        //LOG_ERROR("Unexpected event");
      }
    }
  }
}

bool WebServer::init_socket() {}

void WebServer::init_eventmode(int trigMode) {}

void WebServer::add_client(int fd, sockaddr_in addr) {
  assert(fd > 0);
  m_users[fd].init(fd, addr);
  if(m_timeoutMS > 0) {
    m_timer->add(fd, m_timeoutMS, std::bind(&WebServer::close_conn, this, &m_users[fd]));
  }
  m_epoller->addfd(fd, EPOLLIN | m_conn_event);
  setfd_nonblock(fd);
  //LOG_INFO("Client[%d] in!", users_[fd].GetFd());
}

void WebServer::deal_listen() {
  sockaddr_in addr;
  socklen_t len = sizeof(addr);
  do {
    int fd = accept(m_listenfd, (sockaddr *)&addr, &len);
    if(fd <= 0) return;
    else if(HttpConn::userCount >= MAX_FD) {
      send_error(fd, "Server busy!");
      //LOG_WARN("Clients is full!");
      return;
    }
    add_client(fd, addr);
  } while(m_listen_event & EPOLLET)
}

void WebServer::deal_write(HttpConn *client) {
  assert(client);
  extent_time(client);
  m_threadpool->submit([&] (HttpConn *client) {
    this->on_write(client);
  }, client);
}

void WebServer::deal_read(HttpConn *client) {
  assert(client);
  extent_time(client);
  m_threadpool->submit([&] (HttpConn *client) {
    this->on_read(client);
  }, client);
}

void WebServer::send_error(int fd, const char *info) {
  assert(fd > 0);
  int ret = send(fd, info, strlen(info), 0);
  if(ret < 0) {
    //LOG_WARN("send error to client[%d] error!", fd);
  }
  close(fd);
}

void WebServer::extent_time(HttpConn *client) {
  assert(client);
  if(m_timeoutMS > 0) {
    m_timer->adjust(client->getfd(), m_timeoutMS);
  }
}

void WebServer::close_conn(HttpConn *client) {
  assert(client);
  //LOG_INFO("Client[%d] quit!", client->GetFd());
  m_epoller->delfd(client->getfd());
  client->Close();
}

void WebServer::on_read(HttpConn *client) {}

void WebServer::on_write(HttpConn *client) {}

void WebServer::on_process(HttpConn *client) {}

int WebServer::setfd_nonblock(int fd) {}

} // namespace webserver