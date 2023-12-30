#include <webserver/epoller.h>

namespace webserver {

bool Epoller::addfd(int fd, uint32_t events) {
  if(fd < 0) return false;
  epoll_event ev{};
  ev.data.fd = fd;
  ev.events = events;
  return epoll_ctl(m_epollfd, EPOLL_CTL_ADD, fd, &ev) == 0;
}

bool Epoller::modfd(int fd, uint32_t events) {
  if(fd < 0) return false;
  epoll_event ev{};
  ev.data.fd = fd;
  ev.events = events;
  return epoll_ctl(m_epollfd, EPOLL_CTL_MOD, fd, &ev) == 0;
}

bool Epoller::delfd(int fd) {
  if(fd < 0) return false;
  epoll_event ev{};
  return epoll_ctl(m_epollfd, EPOLL_CTL_DEL, fd, &ev) == 0;
}

int Epoller::wait(int timeoutMs) {
  return epoll_wait(m_epollfd, &m_events[0], static_cast<int>(m_events.size()), timeoutMs);  
}

int Epoller::geteventfd(size_t i) const {
  assert(i < m_events.size() && i >= 0);
  return m_events[i].data.fd;
}

uint32_t Epoller::getevents(size_t i) const {
  assert(i < m_events.size() && i >= 0);
  return m_events[i].events;
}  
} // namespace webserver