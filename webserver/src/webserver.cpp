#include <webserver/webserver.h>
#include <iostream>
namespace webserver {

WebServer::WebServer(int port, int trigMode, int timeoutMS, bool OptLinger,
                     int sqlPort, const char *sqlUser, const char *sqlPwd,
                     const char *dbName, int connPoolNum, int threadNum,
                     bool openLog, int logLevel, int logQueSize) : 
  m_port(port), m_openlinger(OptLinger), m_timeoutMS(timeoutMS), m_isClose(false),
  m_timer(new HeapTimer()), m_threadpool(new ThreadPool(threadNum)), m_epoller(new Epoller()) {
    m_threadpool->init();
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
        assert(m_users.count(fd) > 0);
        deal_read(&m_users[fd]);
      }
      else if(events & EPOLLOUT) {
        assert(m_users.count(fd) > 0);
        deal_write(&m_users[fd]);
      } 
      else {
        //LOG_ERROR("Unexpected event");
      }
    }
  }
}

bool WebServer::init_socket() {
  int ret;
  sockaddr_in addr;
  if(m_port > 65535 || m_port < 1024) {
    //LOG_ERROR("Port:%d error!",  port_);
    return false;
  }
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(m_port);
  linger optLinger = {0};
  if(m_openlinger) {
    optLinger.l_onoff = 1;
    optLinger.l_linger = 1;
  }
  
  m_listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if(m_listenfd < 0) {
    //LOG_ERROR("Create socket error!", port_);
    return false;
  }
  ret = setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
  if(ret < 0) {
    close(m_listenfd);
    //LOG_ERROR("Init linger error!", port_);
    return false;
  }
  int optval = 1;
  ret = setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
  if(ret == -1) {
    //LOG_ERROR("set socket setsockopt error !");
    close(m_listenfd);
    return false;
  }
  ret = bind(m_listenfd, (sockaddr *)&addr, sizeof(addr));
  if(ret < 0) {
    //LOG_ERROR("Bind Port:%d error!", port_);
    close(m_listenfd);
    return false;
  }
  ret = listen(m_listenfd, 6);
  if(ret < 0) {
    //LOG_ERROR("Listen port:%d error!", port_);
    close(m_listenfd);
    return false;
  }
  ret = m_epoller->addfd(m_listenfd, m_listen_event | EPOLLIN);
  if(!ret) {
    //LOG_ERROR("Add listen error!");
    close(m_listenfd);
    return false;
  }
  setfd_nonblock(m_listenfd);
  //LOG_INFO("Server port:%d", port_);
  return true;
}

void WebServer::init_eventmode(int trigMode) {
  m_listen_event = EPOLLRDHUP;
  m_conn_event = EPOLLONESHOT | EPOLLRDHUP;
  switch (trigMode) {
    case 0:
      break;
    case 1:
      m_conn_event |= EPOLLET;
      break;
    case 2:
      m_listen_event |= EPOLLET;
    case 3:
      m_listen_event |= EPOLLET;
      m_conn_event |= EPOLLET;
      break;
    default:
      m_listen_event |= EPOLLET;
      m_conn_event |= EPOLLET;
      break;
  }
  HttpConn::isET = (m_conn_event & EPOLLET);
}

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
  } while(m_listen_event & EPOLLET);
}

void WebServer::deal_write(HttpConn *client) {
  assert(client);
  extent_time(client);
  m_threadpool->submit(std::bind(&WebServer::on_write, this, std::placeholders::_1), client);
}

void WebServer::deal_read(HttpConn *client) {
  assert(client);
  extent_time(client);
  m_threadpool->submit(std::bind(&WebServer::on_read, this, std::placeholders::_1), client);
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

void WebServer::on_read(HttpConn *client) {
  assert(client);
  int ret = -1;
  int readErrno = 0;
  ret = client->read(&readErrno);
  if(ret <= 0 && readErrno != EAGAIN) {
    close_conn(client);
    return;    
  }
  on_process(client);
}

void WebServer::on_write(HttpConn *client) {
  assert(client);
  int ret = -1;
  int writeErrno = 0;
  ret = client->write(&writeErrno);
  if(!client->towritebytes()) {
    if(client->is_keepalive()) {
      on_process(client);
      return;
    }
  } else if(ret < 0) {
    if(writeErrno == EAGAIN) {
      m_epoller->modfd(client->getfd(), m_conn_event | EPOLLOUT);
      return;
    }
  }
  close_conn(client);
}

void WebServer::on_process(HttpConn *client) {
  if(client->process()) {
    m_epoller->modfd(client->getfd(), m_conn_event | EPOLLOUT);
  } else {
    m_epoller->modfd(client->getfd(), m_conn_event | EPOLLIN);
  }
}

int WebServer::setfd_nonblock(int fd) {
  assert(fd > 0);
  return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}
  
} // namespace webserver