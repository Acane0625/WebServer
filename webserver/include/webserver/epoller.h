#pragma once
#include <assert.h> // close()
#include <errno.h>
#include <fcntl.h>     // fcntl()
#include <sys/epoll.h> //epoll_ctl()
#include <unistd.h>    // close()
#include <vector>

namespace webserver {
class Epoller {
private:
  int m_epollfd;
  std::vector<epoll_event> m_events;

public:
  explicit Epoller(int maxEvent = 1024) : m_epollfd(epoll_create(512)), m_events(maxEvent) {
    assert(m_epollfd >= 0 && m_events.size() > 0);
  }
  ~Epoller() {
    close(m_epollfd);
  }
  bool addfd(int fd, uint32_t events);
  bool modfd(int fd, uint32_t events);
  bool delfd(int fd);
  int wait(int timeoutMs = -1);
  int geteventfd(size_t i) const;
  uint32_t getevents(size_t i) const;
};
} // namespace webserver
